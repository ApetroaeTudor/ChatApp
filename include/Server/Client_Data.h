#pragma once

#include <atomic>
#include "resources.h"
#include <string>
#include <string_view>

template<concepts::NonBlockingAtomicStringQueue Queue_t>
struct Client_Data {
    inline static std::atomic<int> id{0};
    int personal_id;

    int fd = -1;
    std::string name;
    std::string_view color = colors::COLOR_ARR[static_cast<size_t>(colors::Colors::WHITE)];
    std::atomic_flag initializing_done = ATOMIC_FLAG_INIT;

    Queue_t send_queue;

    void update_client(int fd_, std::string &&name_, std::string_view color_) {
        this->fd = fd_;
        this->name = std::move(name_);
        this->color = color_;
    }


    explicit Client_Data(int fd = -1) {
        this->fd = fd;
        personal_id = id.fetch_add(1, std::memory_order_relaxed);
    }

    ~Client_Data() = default;

    Client_Data(const Client_Data &other) = delete;

    Client_Data &operator=(const Client_Data &other) = delete;

    Client_Data(Client_Data &&other) = delete;

    Client_Data &operator=(Client_Data &&other) = delete;
};
