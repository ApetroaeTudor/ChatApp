#include "resources.h"
#include <thread>
#include <iostream>
#include <chrono>
#include <sys/epoll.h>
#include "Epoll.h"

#include "Client.h"

void main_loop(Client &cl, EpollManager &em);

void main_loop(Client &cl, EpollManager &em)
{

    int events_nr;
    MessageFrame msg_frame;
    char buffer[constants::MAX_LEN];

    while (1)
    {
        events_nr = em.wait_events();
        if (em.check_event(STDIN_FILENO, EPOLLIN, events_nr))
        {
            try
            {
                while (fgets(buffer, constants::MAX_LEN - 1, stdin) != nullptr)
                {
                    buffer[strcspn(buffer, "\n")] = 0; 
                    if (strcmp(buffer, "#quit") == 0)
                        return;

                    cl.send_msg(buffer);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        else if (em.check_event(cl.get_cl_socket(), EPOLLIN, events_nr))
        {
            try
            {
                cl.receive_msg(msg_frame);
                std::cout << colors::BLUE << "Message Received: " << msg_frame.msg << colors::COL_END << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }

        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

int main()
{

    try
    {
        Client cl(AF_INET, SOCK_STREAM, 0, constants::SV_ADDR, constants::PORT);
        cl.make_cl_socket_nonblocking();

        EpollManager em;
        em.add_monitored_fd(EpollPair{STDIN_FILENO, EPOLLIN});
        em.add_monitored_fd(EpollPair{cl.get_cl_socket(), EPOLLIN});

        main_loop(cl, em);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }

    return 0;
}