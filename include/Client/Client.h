#pragma once

#include <netinet/in.h>
#include <string>
#include <string.h>
#include "../Messages/MessageFrame.h"
#include "Epoll.h"
#include <stop_token>
#include <fcntl.h>
#include <atomic>
#include <mutex>
#include <arpa/inet.h>

#include "Server_Message.h"

class Client {
private:
    std::atomic_flag is_initialized_flag = ATOMIC_FLAG_INIT;
    sockaddr_in s_addr;
    std::mutex data_mutex;

    std::string name;
    std::string_view color;
    std::string id;

    int cl_socket = -1;
    std::stop_source client_done_source{};

    //////////////////////////////////////////////////// -helper functions
    void send_msg_from_stdin(std::string &buffer, Server_Message &msg);

    void receive_msg_from_sv(MessageFrame &msg_frame);

    ////////////////////////////////////////////////////

    void send_msg(std::string &&msg) const;

    int receive_msg(struct MessageFrame &message_frame) const;

public:
    Client(const Client &cl) = delete;

    Client &operator=(const Client &cl) = delete;

    Client(Client &&other) noexcept = delete;

    Client &operator=(Client &&other) noexcept = delete;


    Client(int domain, int type, int protocol, std::string_view sv_addr, int port);

    ~Client();

    void make_cl_socket_nonblocking();

    int get_cl_socket() const;


    bool check_init();

    void set_init();

    void set_name(const char *name);

    std::string get_name() const;

    void set_color(std::string_view color);

    std::string_view get_color() const;

    void set_id(const char *id);
    std::string get_id() const;

    /////////////////////////////////////////////////////// -main loop functions
    void start_send_loop();

    void start_receive_loop();
};
