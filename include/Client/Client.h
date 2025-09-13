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


class Client
{
private:
    struct sockaddr_in s_addr;
    int cl_socket=-1;

public:
    Client(const Client &cl) = delete;
    Client &operator=(const Client &cl) = delete;
    Client(Client &&other) noexcept;
    Client &operator=(Client &&other)noexcept;

    Client(int domain, int type, int protocol, std::string sv_addr, int port);

    ~Client();
       

    void send_msg(std::string &&msg) const ;

    void receive_msg(struct MessageFrame &message_frame) const;

    void make_cl_socket_nonblocking();



    int get_cl_socket() const;
};
