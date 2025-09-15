#include "Client.h"


Client &Client::operator=(Client &&other)noexcept
{
    if (this != &other)
    {
        this->s_addr = other.s_addr;
        memset(&other.s_addr, 0, sizeof(other.s_addr));

        this->cl_socket = other.cl_socket;
        other.cl_socket = -1;
    }
    return *this;
}

Client::Client(Client &&other) noexcept
{
    *this = std::move(other);
}



Client::Client(int domain, int type, int protocol, std::string sv_addr, int port)
{
    this->cl_socket = socket(domain, type, protocol);
    if (cl_socket < 0)
    {
        throw std::runtime_error("Failed to open socket - Client\n");
    }
    if (inet_pton(domain, sv_addr.c_str(), &this->s_addr.sin_addr) <= 0)
    {
        throw std::runtime_error("Failed to load network addr in struct\n");
    }
    this->s_addr.sin_family = domain;
    this->s_addr.sin_port = htons(port);

    if (connect(this->cl_socket, reinterpret_cast<sockaddr *>(&this->s_addr), sizeof(this->s_addr)) < 0)
    {
        throw std::runtime_error("Failed connect\n");
    }
}

Client::~Client()
{
    memset(&this->s_addr, 0, sizeof(this->s_addr));
    close(this->cl_socket);
    this->cl_socket = 0;
}

void Client::send_msg(std::string &&msg) const 
{
    std::string msg_to_send = std::move(msg);
    if (this->cl_socket >= 0)
    {
        if (send(cl_socket, msg_to_send.c_str(), msg_to_send.size(), 0) < 0)
        {
            throw std::runtime_error("Failed to send msg\n");
        }
    }
    else
    {
        throw std::runtime_error("The socket is not initialized\n");
    }
}

void Client::receive_msg(struct MessageFrame &message_frame) const
{
    if (this->cl_socket >= 0)
    {
        if (recv(this->cl_socket, &message_frame.msg, message_frame.msg_len, 0) < 0)
        {
            throw std::runtime_error("Failed read\n");
        }
    }
    throw std::runtime_error("The socket is not initialized\n");
}

void Client::make_cl_socket_nonblocking()
{
    if(cl_socket>=0)
    {
        int flags = fcntl(this->cl_socket,F_GETFL,0);
        if( (flags & O_NONBLOCK) !=O_NONBLOCK)
        {
            fcntl(this->cl_socket,F_SETFL,flags|O_NONBLOCK);

        }
    }
}

int Client::get_cl_socket() const
{
    return this->cl_socket;
}