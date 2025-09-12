#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <stdexcept>
#include "resources.h"

struct MessageFrame
{
    char msg[constants::MAX_LEN];
    int msg_len;
};

class Client
{
private:
    struct sockaddr_in s_addr;
    int cl_socket;

public:
    Client(const Client &cl) = delete;
    Client &operator=(const Client &cl) = delete;
    Client(Client &&other) noexcept;
    Client &operator=(Client &&other);

    Client(int domain, int type, int protocol, std::string sv_addr, int port);

    ~Client();
       

    void send_msg(std::string &&msg) const ;

    void receive_msg(struct MessageFrame &message_frame) const;


    int get_cl_socket() const;
};
