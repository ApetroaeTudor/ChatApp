#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <stdexcept>
#include "MessageFrame.h"
#include <fcntl.h>
#include "Epoll.h"
#include <stop_token>
#include <chrono>

class Client
{
private:
    struct sockaddr_in s_addr;
    int cl_socket = -1;
    std::stop_source client_done_source{};

    //////////////////////////////////////////////////// -helper functions
    void send_msg_from_stdin(std::string &buffer);
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

    /////////////////////////////////////////////////////// -main loop functions
    void start_send_loop();
    void start_receive_loop();
};
