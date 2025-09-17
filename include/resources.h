#pragma once

#include <string_view>
#include <concepts>
#include <string>
#include <array>
#include <utility>


namespace concepts {
    template<typename Q>
    concept NonBlockingAtomicStringQueue = requires(Q q, std::string in, std::string out)
    {
        { q.try_push(std::move(in)) } -> std::same_as<bool>;
        { q.try_pop(out) } -> std::same_as<bool>;
    };
}

namespace constructs {
}

namespace constants {
    constexpr unsigned MAX_LEN = 512;
    constexpr unsigned PORT = 8080;
    constexpr std::string_view SV_ADDR = "127.0.0.1";
    constexpr unsigned MAX_EVENTS = 20;
    constexpr unsigned QUEUE_SIZE = 128;
    constexpr unsigned FINAL_INIT_STAGE = 3;
    constexpr unsigned MAX_CLIENTS = 10;


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

    constexpr std::array<std::string_view, 7> ACTIONS_ARR = {
        {
            {"ACCEPT"},
            {"ACCEPTED"},
            {"INIT_REQ"},
            {"INIT_DONE"},
            {"BROADCAST"},
            {"MSG_RECV"},
            {"CLOSED"}
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

namespace util {
#define ALWAYS_INLINE inline __attribute__((always_inline))
}
