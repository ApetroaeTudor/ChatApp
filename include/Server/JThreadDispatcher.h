#pragma once

#include <iostream>

#include "resources.h"
#include "Server.h"
#include "Epoll.h"
#include "MessageFrame.h"
#include "Server_Message.h"


template<concepts::NonBlockingAtomicStringQueue Queue_t>
class ServerManager {
private:
    std::atomic<int> nr_of_connected_clients{0};
    std::atomic<int> max_clients_per_thread{1};
    const int nr_of_worker_threads = 3;

    Queue_t worker_to_listener;
    Queue_t listener_to_worker;

    std::array<std::optional<std::pair<int, std::jthread> >, constants::MAX_NR_OF_THREADS> worker_threads;


    void worker_loop(IServer<Queue_t> &isv, int cl_fd, int cl_id) {
        EpollManager em;
        std::vector<std::pair<int, int> > clients = {{cl_fd, cl_id}};
        MessageFrame msg_frame;
        msg_frame.msg_len = constants::MAX_LEN;

        em.add_monitored_fd(EpollPair{cl_fd, cl_id});
        int ret = -1;
        while (!clients.empty()) {
            ret = em.wait_events();
            if (ret > 0) {
                for (const auto &client: clients) {
                    if (em.check_event(client.first,EPOLLIN, ret)) {
                        if (isv.receive_msg(msg_frame, client.second) == 0) {
                            // client exit
                            isv.disconnect_client(client.second);
                            // send message to server Queue
                            clients.erase(std::remove_if(clients.begin(), clients.end(),
                                                         [&client](std::pair<int, int> a) {
                                                             return a.second == client.second;
                                                         }), clients.end());
                            continue;
                            // if stage == 0 take init message -> stage = 1
                            // if stage == 2 take init fin message -> stage = 3
                            // if stage == 3 take message and put on worker_to_listener queue

                            // client has a valid msg in the buffer
                            // put msg in worker_to_listener
                        }
                        if (em.check_event(client.first,EPOLLOUT, ret)) {
                            // can send a message to client
                            // if stage == 1, send init msg -> stage = 2
                            // if stage == 3 : check listener_to_worker, send message to client if BROADCAST
                        }
                    }
                }
            }
        }
    }


    void listener_loop(IServer<Queue_t> &isv) {
        try {
            // must check server queue to print messages to server stdout

            // must check server stdin to exit
            // must check worker_to_listener to check for disconnects and broadcasts
            // must write to listener_to_worker to send broadcasts and new clients
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
