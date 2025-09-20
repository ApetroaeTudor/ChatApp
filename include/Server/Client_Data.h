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

    std::atomic<int> init_state{constants::INITIAL_STATE};

    Queue_t send_queue;

    void inc_state() {
        this->init_state.fetch_add(1, std::memory_order_release);
    }
    int get_state() {
        return this->init_state.load(std::memory_order_acquire);
    }


    void update_client(int fd_, std::string &&name_, std::string_view color_) {
        this->fd = fd_;
        this->name = std::move(name_);
        this->color = color_;
    }


    explicit Client_Data(int fd = -1) {
        this->fd = fd;
        personal_id = id.fetch_add(1, std::memory_order_relaxed);
        this->color = utils::getRandomColor();
    }

    ~Client_Data() = default;

    Client_Data(const Client_Data &other) = delete;

    Client_Data &operator=(const Client_Data &other) = delete;

    Client_Data(Client_Data &&other) = delete;

    Client_Data &operator=(Client_Data &&other) = delete;
};
