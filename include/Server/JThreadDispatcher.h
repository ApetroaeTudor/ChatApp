#pragma once

#include <iostream>

#include "resources.h"
#include "Server.h"
#include "Epoll.h"

template<concepts::NonBlockingAtomicStringQueue Queue_t>
class ServerManager {
private:
    std::atomic<int> nr_of_connected_clients{0};
    std::atomic<int> max_clients_per_thread{1};
    const int nr_of_worker_threads = 3;
    Queue_t broadcast_queue;

    EpollManager epoll_manager{};

    void listener_loop(IServer<Queue_t> &isv) {
        std::string buffer;
        epoll_manager.add_monitored_fd(EpollPair{STDIN_FILENO,EPOLLIN});

        while (isv.check_is_done()) {
            if (int ret = epoll_manager.wait_events(); epoll_manager.check_event(STDIN_FILENO,EPOLLIN, ret)) {
                if (std::getline(std::cin, buffer)) {
                    if (buffer == "#quit") {
                        isv.set_is_done();
                    }
                }
            }
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
        try {
            return std::jthread([&]() { listener_loop(isv); });
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
};
