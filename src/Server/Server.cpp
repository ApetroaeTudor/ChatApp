#include "Server.h"




Server::Server(int domain, int type, int protocol, int port, std::string &LOCALHOST, int sock_opts, int optval)
{
    
}

Server::Server(int domain, int type, int protocol, int port, int sock_opts, int optval)


void Server::listen_connections(int max_connections) const


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

