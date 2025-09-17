#pragma once

#include <iostream>

#include "resources.h"
#include "Server.h"
#include "Epoll.h"
#include "MessageFrame.h"

template<concepts::NonBlockingAtomicStringQueue Queue_t>
class ServerManager {
private:
    std::atomic<int> nr_of_connected_clients{0};
    std::atomic<int> max_clients_per_thread{1};
    const int nr_of_worker_threads = 3;
    Queue_t broadcast_queue;
    std::pair<int,int> cl_pair{-1,-1};



    void listener_loop(IServer<Queue_t> &isv) {
        try {
            EpollManager epoll_manager{};
            std::string buffer;
            int sv_socket = isv.get_sv_socket();
            epoll_manager.add_monitored_fd(EpollPair{STDIN_FILENO,EPOLLIN});
            epoll_manager.add_monitored_fd(EpollPair{sv_socket,EPOLLIN});
            while (!isv.check_is_done()) {
                int ret = epoll_manager.wait_events();
                if (epoll_manager.check_event(STDIN_FILENO,EPOLLIN, ret)) {
                    if (std::getline(std::cin, buffer)) {
                        if (buffer == "#quit") {
                            isv.set_is_done();
                        }
                    }
                }
                if (epoll_manager.check_event(sv_socket,EPOLLIN, ret)) {
                    std::cout<<"Currently there are "<<isv.get_nr_of_connections()<<" Connected clients before the new one"<<std::endl;
                    if (cl_pair = isv.accept_client(true); cl_pair.first>=0) {
                        epoll_manager.add_monitored_fd(EpollPair{cl_pair.first,EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP});
                    }
                }

                MessageFrame msg;
                msg.msg_len = constants::MAX_LEN;
                if (cl_pair.first >= 0) {
                    if (epoll_manager.check_event(cl_pair.first,EPOLLIN, ret)) {
                        if (isv.receive_msg(msg,cl_pair.second) == 0) {
                            isv.disconnect_client(cl_pair.second);
                        }
                    }
                }

            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }

public:
    ServerManager() = default;

    ServerManager &operator=(const ServerManager &other) = delete;

    ServerManager(const ServerManager &other) = delete;

    ServerManager &operator=(ServerManager &&other) noexcept = delete;

    ServerManager(ServerManager &&other) noexcept = delete;

    Queue_t received_msg_queue;

    std::jthread launch_listener_loop(IServer<Queue_t> &isv) {
        return std::jthread([&]() { listener_loop(isv); });
    }
};
