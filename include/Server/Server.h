#pragma once

#include "MessageFrame.h"
#include "resources.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <fcntl.h>
#include <atomic>
#include <thread>
#include <stop_token>
#include <algorithm>

#include "atomic_queue.h"

// using AtomicQueueString = atomic_queue::AtomicQueue2<std::string, std::allocator<std::string>, constants::MAX_QUEUE>;

template <concepts::NonBlockingAtomicStringQueue Queue_t>
struct Client_Data;

template <concepts::NonBlockingAtomicStringQueue Queue_t>
class IServer
{
public:
    virtual int get_sv_socket() const override = 0;
    virtual Client_Data<Queue_t> &get_client_ref(int cl_id) = 0;
    virtual void add_client(std::array<Client_Data<Queue_t>, constants::MAX_CLIENTS> &clients, int fd) = 0;

    virtual ~IServer() = default;
};

namespace concepts
{
    template <typename D, typename Q>
    concept ListenerLauncher = NonBlockingAtomicStringQueue<Q> && requires(D d, IServer<Q> sv) {
        { d.launch_listener_loop(d) };
    };
}

template <concepts::NonBlockingAtomicStringQueue Queue_t>
struct Client_Data
{
    inline static std::atomic<int> id{0};
    const int personal_id;

    int fd = -1;
    std::string name;
    std::string_view color = colors::WHITE;
    std::atomic<int> init_step{0};
    std::atomic_flag initializing_done = ATOMIC_FLAG_INIT;
    Queue_t send_queue;

    void update_client(int fd, std::string &&name, std::string_view color)
    {
        this->id = id;
        this->fd = fd;
        this->name = std::move(name);
        this->color = color;
    }

    void inc_init_stage()
    {
        int previous_step = this->init_step.fetch_add(1, std::memory_order_release);
        if (previous_step + 1 == constants::FINAL_INIT_STAGE)
        {
            this->initializing_done.test_and_set(std::memory_order_release);
        }
    }

    Client_Data()
    {
        personal_id = id.fetch_add(1, std::memory_order_relaxed);
    }

    ~Client_Data() = default;
    Client_Data(const Client_Data &other) = delete;
    Client_Data &operator=(const Client_Data &other) = delete;
    Client_Data(Client_Data &&other) = delete;
    Client_Data &operator=(Client_Data &&other) = delete;
};

template <concepts::NonBlockingAtomicStringQueue Queue_t, concepts::ListenerLauncher<Queue_t> Server_Manager_t>
class Server : public IServer<Queue_t>
{
private:
    struct sockaddr_in s_addr;
    int sv_socket = -1;
    std::array<Client_Data<Queue_t>, constants::MAX_CLIENTS> clients;
    std::stop_source server_done_source{};

    Server_Manager_t sv_manager{};

public:
    Server(const Server &sv) = delete;
    Server &operator=(const Server &sv) = delete;

    Server(Server &&other) noexcept = delete;
    Server &operator=(Server &&other) noexcept = delete;

    Server(int domain, int type, int protocol, int port, std::string_view sv_addr, int sock_opts, int optval);
    Server(int domain, int type, int protocol, int port, int sock_opts, int optval);

    ~Server();

    void listen_connections(int max_connections) const;

    int add_client_socket(int make_non_blocking);

    void make_sv_socket_nonblocking();

    ///////////////////////////////////////////////////////////////////// -- interface functions

    int get_sv_socket() const override
    {
        return this->sv_socket;
    }

    Client_Data<Queue_t> &get_client_ref(int cl_id) override
    {
    }
    void add_client(std::array < Client_Data<Queue_t>, constants::MAX_CLIENTS> & clients, int fd) override
    {
    }
};
