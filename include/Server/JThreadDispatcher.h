#pragma once

#include "resources.h"
#include "Server.h"

template <concepts::NonBlockingAtomicStringQueue Queue_t>
class ServerManager
{
private:
    std::atomic<int> nr_of_connected_clients{0};
    std::atomic<int> max_clients_per_thread{1};
    const int nr_of_worker_threads = 3;
    Queue_t broadcast_queue;

    void send_msg(std::string &&msg, int cl_id) const override
    {
    }
    int receive_msg(MessageFrame &message_frame, int cl_id) const override
    {
    }

    void listener_loop(Server &sv)
    {
    }

public:
    ServerManager() = default;
    ServerManager &operator=(const ServerManager &other) = delete;
    ServerManager(const ServerManager &other) = delete;
    ServerManager &operator=(ServerManager &&other) noexcept = delete;
    ServerManager(ServerManager &&other) noexcept = delete;
    Queue_t received_msg_queue;

    std::jthread launch_listener_loop(Server &sv)
    {
        return std::jthread([&]()
                            { listener_loop(sv) });
    }
};