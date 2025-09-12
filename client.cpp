#include "resources.h"
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>

int main()
{
    
    struct sockaddr_in s_addr;
    int cl_socket;
    cl_socket = socket(AF_INET,SOCK_STREAM,0);
    if(cl_socket<0)
    {
        perror("Failed to open socket\n");
        exit(EXIT_FAILURE);
    }


    if(inet_pton(AF_INET,constants::SV_ADDR,&s_addr.sin_addr)<=0)
    {
        perror("Failed to load network addr in struct\n");
        exit(EXIT_FAILURE);
    }

    s_addr.sin_family=AF_INET;
    s_addr.sin_port  = htons(constants::PORT); 



    if(connect(cl_socket,reinterpret_cast<sockaddr*>(&s_addr),sizeof(s_addr)) <0) 
    {
        perror("Failed connect\n");
        exit(EXIT_FAILURE);
    }


    std::string msg = "hey";

    if(send(cl_socket,msg.c_str(),msg.size(),0 ) <0)
    {
        perror("Failed send\n");
        exit(EXIT_FAILURE);
    }


    close(cl_socket);
    return 0;
}