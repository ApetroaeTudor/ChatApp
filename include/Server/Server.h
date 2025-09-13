#pragma once

#include "MessageFrame.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include <vector>
#include <arpa/inet.h>
#include <stdexcept>
#include <fcntl.h>

class Server
{
private:
    struct sockaddr_in s_addr; 
    int sv_socket=-1;
    std::vector<int> clients;
public:

    Server(const Server& sv) = delete;
    Server& operator=(const Server& sv) = delete;

    Server(Server && other) noexcept;
    Server& operator=(Server&& other)noexcept;

    Server(int domain, int type, int protocol, int port, std::string& sv_addr, int sock_opts, int optval);
    Server(int domain, int type, int protocol, int port, int sock_opts, int optval);

    void listen_connections(int max_connections)const;

    int add_client_socket(int make_non_blocking);

    int get_sv_socket()const;

    void make_sv_socket_nonblocking();


    void send_msg(std::string&& msg, int cl_id)const;
    void receive_msg(struct MessageFrame &message_frame, int cl_id)const;
};