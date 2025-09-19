#pragma once

#include <string_view>
#include <concepts>
#include <iostream>
#include <string>
#include <array>
#include <utility>
#include <chrono>
#include <sys/eventfd.h>



namespace concepts {
    template<typename Q>
    concept NonBlockingAtomicStringQueue = requires(Q q, std::string in, std::string out)
    {
        { q.try_push(std::move(in)) } -> std::same_as<bool>;
        { q.try_pop(out) } -> std::same_as<bool>;
    };
}


namespace constants {
    constexpr unsigned MAX_LEN = 512;

    constexpr unsigned MAX_NR_OF_THREADS = 2;
    constexpr unsigned MAX_CLIENTS_PER_THREAD = 3;

    constexpr unsigned PORT = 8080;
    constexpr std::string_view SV_ADDR = "127.0.0.1";
    constexpr unsigned MAX_EVENTS = 20;
    constexpr unsigned QUEUE_SIZE = 128;
    constexpr unsigned FINAL_INIT_STAGE = 3;
    constexpr unsigned MAX_CLIENTS = MAX_NR_OF_THREADS*MAX_CLIENTS_PER_THREAD;


    constexpr const char *NAME_UNINITIALIZED = "name_uninitialized";
    constexpr const char *ID_UNINITIALIZED = "id_uninitialized";
    constexpr const char *ANY_THREAD = "any_thread";

    enum class Actions: size_t {
        ACCEPT = 0,
        ACCEPTED = 1,
        INIT_REQ = 2,
        INIT_DONE = 3,
        BROADCAST = 4,
        MSG_RECV = 5,
        CLOSED = 6
    };

    constexpr std::array<const char*, 7> ACTIONS_ARR = {
        {
            "ACCEPT",
            "ACCEPTED",
            "INIT_REQ",
            "INIT_DONE",
            "BROADCAST",
            "MSG_RECV",
            "CLOSED"
        }
    };
}

namespace colors {
    enum class Colors : size_t {
        COL_END = 0,
        RED = 1,
        GREEN = 2,
        YELLOW = 3,
        BLUE = 4,
        MAGENTA = 5,
        CYAN = 6,
        WHITE = 7,

        BRIGHT_RED = 8,
        BRIGHT_GREEN = 9,
        BRIGHT_YELLOW = 10,
        BRIGHT_BLUE = 11,
        BRIGHT_MAGENTA = 12,
        BRIGHT_CYAN = 13,
        BRIGHT_WHITE = 14,
    };


    constexpr std::array<std::string_view, 15> COLOR_ARR = {
        {
            {"\033[0m"}, //0 end

            {"\033[31m"}, //1 red
            {"\033[32m"}, //2 green
            {"\033[33m"}, //3 yellow
            {"\033[34m"}, //4 blue
            {"\033[35m"}, // 5 magenta
            {"\033[36m"}, // 6 cyan
            {"\033[37m"}, // 7 white


            {"\033[91m"}, // 8 bright red
            {"\033[92m"}, // 9 bright green
            {"\033[93m"}, // 10 bright yellow
            {"\033[94m"}, // 11 bright blue
            {"\033[95m"}, //12 bright magenta
            {"\033[96m"}, //13 bright cyan
            {"\033[97m"} //14 bright white
        }
    };
}

namespace characters {
    constexpr char BACKSPACE = 8;
}


namespace utils {

    #define ALWAYS_INLINE inline __attribute__((always_inline))


    ALWAYS_INLINE constexpr auto time_millis(int time_millis) {
        return std::chrono::milliseconds(time_millis);
    }

    class ServerFullException: public std::runtime_error {
    public:
        explicit ServerFullException(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    class QueueException: public std::runtime_error {
    public:
        explicit QueueException(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    class ClientConnectionException: public std::runtime_error {
        public:
        explicit ClientConnectionException(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    ALWAYS_INLINE int cast_int(auto i) {
        return static_cast<int>(i);
    }

    ALWAYS_INLINE constexpr std::string_view get_color(colors::Colors color_type) {
        return colors::COLOR_ARR[static_cast<size_t>(color_type)];
    }

    ALWAYS_INLINE constexpr const char* get_action(constants::Actions action_type) {
        return constants::ACTIONS_ARR[static_cast<size_t>(action_type)];
    }

    ALWAYS_INLINE int get_evfd(int flags) {
        int evfd = eventfd(0,flags);
        if (evfd<0) {
            throw std::runtime_error("eventfd error\n");
        }
        return evfd;
    }

    ALWAYS_INLINE void cerr_out_red(const char* msg) {
        std::cerr << utils::get_color(colors::Colors::BRIGHT_RED) << msg << utils::get_color(
                            colors::Colors::COL_END) << std::endl;
    }

}


