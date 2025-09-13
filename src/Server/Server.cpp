#include "Server.h"

Server &Server::operator=(Server &&other) noexcept
{
    if (this != &other)
    {
        this->s_addr = other.s_addr;
        memset(&other.s_addr, 0, sizeof(other.s_addr));
        this->sv_socket = other.sv_socket;
        other.sv_socket = -1;
        this-> clients = std::move(other.clients);

    }
    return *this;
}

Server::Server(Server &&other) noexcept
{
    *this = std::move(other);
}

Server::Server(int domain, int type, int protocol, int port, std::string &sv_addr, int sock_opts, int optval)
{
    if (inet_pton(domain, sv_addr.c_str(), &this->s_addr.sin_addr) <= 0)
    {
        throw std::runtime_error("Failed to load network addr in struct\n");
    }
    this->s_addr.sin_family = domain;
    this->s_addr.sin_port = htons(port);
    this->sv_socket = socket(domain, type, protocol);
    if (this->sv_socket < 0)
    {
        throw std::runtime_error("Failed to open socket - Server\n");
    }

    if (setsockopt(sv_socket, SOL_SOCKET, sock_opts, &optval, sizeof(optval)) < 0)
    {
        throw std::runtime_error("Failed to use setsockopt - Server\n");
    }

    if (bind(sv_socket, reinterpret_cast<sockaddr *>(&s_addr), sizeof(s_addr)) < 0)
    {
        throw std::runtime_error("Failed to bind - Server\n");
    }
}

Server::Server(int domain, int type, int protocol, int port, int sock_opts, int optval)
{
    this->s_addr.sin_addr.s_addr = INADDR_ANY;
    this->s_addr.sin_family = domain;
    this->s_addr.sin_port = htons(port);

    this->sv_socket = socket(domain, type, protocol);
    if (this->sv_socket < 0)
    {
        throw std::runtime_error("Failed to open socket - Server\n");
    }

    if (setsockopt(sv_socket, SOL_SOCKET, sock_opts, &optval, sizeof(optval)) < 0)
    {
        throw std::runtime_error("Failed to use setsockopt - Server\n");
    }

    if (bind(sv_socket, reinterpret_cast<sockaddr *>(&s_addr), sizeof(s_addr)) < 0)
    {
        throw std::runtime_error("Failed to bind - Server\n");
    }
}

void Server::listen_connections(int max_connections) const
{
    if (listen(this->sv_socket, max_connections) < 0)
    {
        throw std::runtime_error("Failed to listen - Server\n");
    }
}

int Server::add_client_socket(int make_non_blocking)
{
    socklen_t addr_len = sizeof(s_addr);

    if(this->sv_socket>=0)
    {
        int cl_socket = accept(sv_socket,(sockaddr*)(&s_addr),&addr_len);
        if(cl_socket<0)
        {
            throw std::runtime_error("Failed to generate an accepted client socket\n");
        }
        if(make_non_blocking)
        {
            int flags = fcntl(cl_socket,F_GETFL,0);
            fcntl(cl_socket,F_SETFL,flags | O_NONBLOCK);
        }
        int cl_id = this->clients.size();
        this->clients.push_back(cl_socket);
        return cl_id;
    }
    throw std::runtime_error("The sv fd is not valid!\n");
}

void Server::make_sv_socket_nonblocking()
{
    if(sv_socket>=0)
    {
        int flags = fcntl(this->sv_socket,F_GETFL,0);
        if( (flags & O_NONBLOCK) !=O_NONBLOCK)
        {
            fcntl(this->sv_socket,F_SETFL,flags|O_NONBLOCK);

        }
    }
}



int Server::get_sv_socket() const
{
    return this->sv_socket;
}

void Server::send_msg(std::string&& msg, int cl_id)const
{
    if(cl_id<0 || cl_id>=this->clients.size())
    {
        throw std::runtime_error("Invalid client id\n");
    }
    int cl_fd = this->clients[cl_id];

    std::string msg_to_send = std::move(msg);
    if(this->sv_socket>=0)
    {
        if(send(cl_fd,msg_to_send.c_str(),msg_to_send.size(),0)<0)
        {
            throw std::runtime_error("Failed to send msg\n");
        }
    }
    else
    {
        throw std::runtime_error("The server socket is uninitialized\n");
    }
}

void Server::receive_msg(struct MessageFrame &message_frame, int cl_id) const
{
    if(cl_id<0 || cl_id>=this->clients.size())
    {
        throw std::runtime_error("Invalid client id\n");
    }
    int cl_fd = this->clients[cl_id];

    if(sv_socket>=0)
    {
        if(recv(cl_fd,&message_frame.msg,message_frame.msg_len,0)<0)
        {
            throw std::runtime_error("Failed receive\n");
        }
    }
    else
    {
        throw std::runtime_error("The server socket is uninitialized\n");
    }
}

