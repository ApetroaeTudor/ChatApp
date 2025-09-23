#pragma once

#include "../Messages/MessageFrame.h"
#include "resources.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <stdexcept>
#include <fcntl.h>
#include <thread>
#include <stop_token>
#include <algorithm>
#include <optional>
#include <functional>
#include <Client_Data.h>
#include "atomic_queue.h"
#include <utility>


template<concepts::Q_ThrS_Str Queue_t>
class IServer {
public:
    [[nodiscard]] virtual int get_sv_socket() const = 0;

    virtual std::optional<std::reference_wrapper<Client_Data<Queue_t> > > get_client_ref(int cl_id) = 0;

    virtual std::vector<int> get_connected_IDs() = 0;

    virtual std::pair<int, int> accept_client(bool non_blocking) = 0;

    virtual void disconnect_client(int cl_id) = 0;

    virtual int get_nr_of_connections() = 0;

    virtual void send_msg(std::string &&msg, int cl_id) = 0;

    virtual ssize_t receive_msg(MessageFrame &message_frame, int cl_id) = 0;

    virtual ~IServer() = default;


    [[nodiscard]] virtual bool check_is_done() const = 0;

    virtual void set_is_done() const =0;
};

namespace concepts {
    template<typename D, typename Q>
    concept ListenerLauncher = Q_ThrS_Str<Q> && requires(D d, IServer<Q> sv)
    {
        { d.launch_listener_loop(sv) };
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<concepts::Q_ThrS_Str Queue_t, concepts::ListenerLauncher<Queue_t> Server_Manager_t>
class Server final : public IServer<Queue_t> {
private:
    sockaddr_in s_addr{};
    int sv_socket = -1;
    std::atomic<int> nr_of_connections{0};
    std::array<std::optional<Client_Data<Queue_t> >, constants::MAX_CLIENTS> clients{};
    std::stop_source server_done_source{};
    Server_Manager_t sv_manager{};

    int get_selected_client_fd(int cl_id) {
        if (this->sv_socket < 0) {
            throw std::runtime_error("SV socket not initialized\n");
        }

        auto selected_client = this->get_client_ref(cl_id);
        if (!selected_client.has_value()) {
            throw std::runtime_error("Client not available\n");
        }

        int selected_fd = selected_client.value().get().fd;
        if (selected_fd < 0) {
            throw std::runtime_error("Invalid client ID\n");
        }

        return selected_fd;
    }

public:
    Server(const Server &sv) = delete;

    Server &operator=(const Server &sv) = delete;

    Server(Server &&other) noexcept = delete;

    Server &operator=(Server &&other) noexcept = delete;

    Server(int domain, int type, int protocol, int port, std::string_view LOCALHOST, int sock_opts, int optval) {
        if (inet_pton(domain, LOCALHOST.data(), &this->s_addr.sin_addr) <= 0) {
            throw std::runtime_error("Failed to load network addr in struct\n");
        }
        this->s_addr.sin_family = domain;
        this->s_addr.sin_port = htons(port);
        this->sv_socket = socket(domain, type, protocol);
        if (this->sv_socket < 0) {
            throw std::runtime_error("Failed to open socket - Server\n");
        }

        if (setsockopt(sv_socket, SOL_SOCKET, sock_opts, &optval, sizeof(optval)) < 0) {
            throw std::runtime_error("Failed to use setsockopt - Server\n");
        }

        if (bind(sv_socket, reinterpret_cast<sockaddr *>(&s_addr), sizeof(s_addr)) < 0) {
            throw std::runtime_error("Failed to bind - Server\n");
        }
    }

    Server(int domain, int type, int protocol, int port, int sock_opts, int optval) {
        this->s_addr.sin_addr.s_addr = INADDR_ANY;
        this->s_addr.sin_family = domain;
        this->s_addr.sin_port = htons(port);

        this->sv_socket = socket(domain, type, protocol);
        if (this->sv_socket < 0) {
            throw std::runtime_error("Failed to open socket - Server\n");
        }

        if (setsockopt(sv_socket, SOL_SOCKET, sock_opts, &optval, sizeof(optval)) < 0) {
            throw std::runtime_error("Failed to use setsockopt - Server\n");
        }

        if (bind(sv_socket, reinterpret_cast<sockaddr *>(&s_addr), sizeof(s_addr)) < 0) {
            throw std::runtime_error("Failed to bind - Server\n");
        }
    }

    ~Server() override {
        for (auto &cl: this->clients) {
            if (cl.has_value()) {
                close(cl->fd);
            }
        }
        close(sv_socket);
    }

    void listen_connections() {
        if (listen(this->sv_socket, constants::MAX_CLIENTS) < 0) {
            throw std::runtime_error("Failed to listen - Server\n");
        }
    }

    int get_nr_of_connections() override {
        return this->nr_of_connections.load(std::memory_order_relaxed);
    };


    void make_sv_socket_nonblocking() {
        if (sv_socket < 0) {
            throw std::runtime_error("Sv socket not initialized\n");
        }

        int flags = fcntl(this->sv_socket, F_GETFL, 0);
        if (flags < 0) {
            throw std::runtime_error("Could not fetch flags\n");
        }

        if ((flags & O_NONBLOCK) != O_NONBLOCK) {
            if (fcntl(this->sv_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
                throw std::runtime_error("Failed fcntl call\n");
            }
        }
    }


    std::jthread launch_server_loop() {
        return this->sv_manager.launch_listener_loop(*this);
    }

    ///////////////////////////////////////////////////////////////////// -- interface functions

    [[nodiscard]] int get_sv_socket() const override {
        return this->sv_socket;
    }


    std::vector<int> get_connected_IDs() override {
        std::vector<int> IDs;
        for (const auto &cl: this->clients) {
            if (cl.has_value()) {
                IDs.push_back(cl->personal_id);
            }
        }
        return IDs;
    }

    std::optional<std::reference_wrapper<Client_Data<Queue_t> > > get_client_ref(int cl_id) override {
        auto it = std::find_if(this->clients.begin(), this->clients.end(),
                               [&cl_id](const std::optional<Client_Data<Queue_t> > &client) {
                                   return (client.has_value() && client.value().personal_id == cl_id);
                               });
        if (it != this->clients.end()) {
            return std::ref(**it);
        }
        return std::nullopt;
    }

    void disconnect_client(int cl_id) override {
        auto selected_client_optional_cpy = this->get_client_ref(cl_id);
        if (!selected_client_optional_cpy.has_value()) {
            throw std::runtime_error("Client not found\n");
        }
        close(selected_client_optional_cpy.value().get().fd);

        for (auto &client: this->clients) {
            if (client.has_value() && client->personal_id == cl_id) {
                this->nr_of_connections.fetch_add(-1, std::memory_order_release);
                client.reset();
                return;
            }
        }
    }

    std::pair<int, int> accept_client(const bool make_non_blocking) override {
        if (this->sv_socket < 0) {
            throw std::runtime_error("SV socket not initialized\n");
        }
        socklen_t addr_len = sizeof(s_addr);
        int cl_socket = accept4(sv_socket, reinterpret_cast<sockaddr *>(&s_addr), &addr_len,SOCK_NONBLOCK);
        if (cl_socket < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return {-1, -1};
            }
            throw std::runtime_error("Failed to accept client\n");
        }
        if (make_non_blocking) {
            const int flags = fcntl(cl_socket, F_GETFL, 0);
            if (flags < 0) {
                // TODO: send err msg to client
                close(cl_socket);
                throw std::runtime_error("Failed to get flags on client\n");
            }
            if (fcntl(cl_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
                // TODO: send err msg to client
                close(cl_socket);
                throw std::runtime_error("Failed fcntl call\n");
            }
        }

        auto free_slot = std::find_if(this->clients.begin(), this->clients.end(),
                                      [](const auto &opt_it) { return !opt_it.has_value(); });

        if (free_slot != this->clients.end()) {
            free_slot->emplace(cl_socket);
            this->nr_of_connections.fetch_add(1, std::memory_order_release);
            return {cl_socket, free_slot->value().personal_id};
        } else {
            // TODO: send a deny message to client
            close(cl_socket); // no place for the client
        }
        return {-1, -1};
    }


    [[nodiscard]] bool check_is_done() const override {
        return this->server_done_source.stop_requested();
    }

    void set_is_done() const override {
        if (!this->server_done_source.request_stop()) {
            throw std::runtime_error("Server did not finish\n");
        }
    }

    //////////////////////////////////////////////////////////////////////////////// send,receive

    void send_msg(std::string &&msg, int cl_id) override {
        int selected_fd = get_selected_client_fd(cl_id);
        std::string msg_to_send = std::move(msg);
        if (send(selected_fd, msg_to_send.c_str(), msg_to_send.size(), 0) < 0) {
            throw std::runtime_error("Failed to send msg\n");
        }
    }

    ssize_t receive_msg(MessageFrame &message_frame, int cl_id) override {
        int selected_fd = get_selected_client_fd(cl_id);
        ssize_t received_msg_len = recv(selected_fd, &message_frame.msg, message_frame.msg_len, 0);
        if (received_msg_len < 0) {
            throw std::runtime_error("Failed to receive msg\n");
        }
        return received_msg_len;
    }
};
