#include "resources.h"
#include <thread>
#include <iostream>
#include <chrono>
#include <sys/epoll.h>

#include "Client.h"

void main_loop(Client& cl, int epfd);



void main_loop(Client& cl, int epfd)
{

    struct epoll_event evlist[constants::MAX_EVENTS];
    int i;
    char buffer[constants::MAX_LEN];
    int ret;
    MessageFrame msg_frame;

    while(1)
    {
        ret = epoll_wait(epfd,evlist,constants::MAX_EVENTS,0);
        if(ret <0)
        {
            perror("Epoll wait err\n");
            exit(EXIT_FAILURE);
        }
        for(i=0;i<ret;i++)
        {
            if(evlist[i].data.fd == STDIN_FILENO)
            {
                fgets(buffer,constants::MAX_LEN-1,stdin);
                if(buffer[strlen(buffer)-1]=='\n')
                {
                    buffer[strlen(buffer)-1]='\0';
                }

                if(strcmp(buffer,"#quit\0")==0)
                {
                    return;
                }
                try{
                    
                    cl.send_msg(buffer);
                    memset(buffer,0,sizeof(buffer));
                }
                catch(const std::exception& e)
                {
                    std::cerr<<e.what()<<'\n';
                }
            }
            else if(evlist[i].data.fd == cl.get_cl_socket())
            {
                try
                {
                    cl.receive_msg(msg_frame);
                    std::cout<<colors::BLUE<<"Message Received: "<<msg_frame.msg<<colors::COL_END<<std::endl;

                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                }
                
            }
        }

        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}


int main()
{

   


    try
    {
        Client cl(AF_INET,SOCK_STREAM,0,constants::SV_ADDR,constants::PORT);

        int epfd = epoll_create1(0);
        if(epfd<0)
        {
            perror("Failed to call epoll_create\n");
            exit(EXIT_FAILURE);
        }

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = STDIN_FILENO;
        int epoll_ret = epoll_ctl(epfd,EPOLL_CTL_ADD,STDIN_FILENO,&ev);
        if(epoll_ret<0)
        {
            perror("Failed to call epoll_ctl nr1\n");
            exit(EXIT_FAILURE);
        }
        epoll_ret = epoll_ctl(epfd,EPOLL_CTL_ADD,cl.get_cl_socket(),&ev);
        if(epoll_ret<0)
        {
            perror("Failed to call epoll_ctl nr2\n");
            exit(EXIT_FAILURE);
        }
        

        main_loop(cl,epfd);



    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    

    return 0;
}