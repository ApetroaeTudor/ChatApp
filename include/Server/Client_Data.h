#pragma once

#include <atomic>
#include "resources.h"
#include <string>
#include <string_view>

template<concepts::Q_ThrS_Str Queue_t>
struct Client_Data {
    inline static std::atomic<int> id{0};
    int personal_id;

    int fd = -1;
    std::string name;
    int color_id{static_cast<int>(colors::Colors::WHITE)};
    std::string_view color = colors::COLOR_ARR[static_cast<size_t>(colors::Colors::WHITE)];

    int init_state{constants::INITIAL_STATE};

    Queue_t send_queue;

    void inc_state() {
        ++this->init_state;
    }
    int get_state() {
        return this->init_state;
    }

    void set_state(int new_state) {
        this->init_state = new_state;
    }


    void update_client(int fd_, std::string &&name_, int color_id) {
        this->fd = fd_;
        this->name = std::move(name_);
        this->color_id = color_id;
    }


    explicit Client_Data(int fd = -1) {
        this->fd = fd;
        personal_id = id.fetch_add(1, std::memory_order_relaxed);
        this->color_id = utils::getRandomColorID();
    }

    ~Client_Data() = default;

    Client_Data(const Client_Data &other) = delete;

    Client_Data &operator=(const Client_Data &other) = delete;

    Client_Data(Client_Data &&other) = delete;

    Client_Data &operator=(Client_Data &&other) = delete;
};
