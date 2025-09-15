#pragma once

#include <string_view>
#include <concepts>


namespace concepts
{
    template <typename Q>
    concept NonBlockingAtomicStringQueue = requires(Q q, std::string in, std::string out) {
        { q.try_push(std::move(in)) } -> std::same_as<bool>;
        { q.try_pop(out) } -> std::same_as<bool>;
    };


}

namespace constructs
{

}

namespace constants
{
    constexpr unsigned MAX_LEN = 200;
    constexpr unsigned PORT = 8080;
    constexpr std::string_view SV_ADDR = "127.0.0.1";
    constexpr unsigned MAX_EVENTS = 20;
    constexpr unsigned QUEUE_SIZE = 16;
    constexpr unsigned FINAL_INIT_STAGE = 3;
    constexpr unsigned MAX_CLIENTS = 10;
}

namespace colors
{
    constexpr std::string_view RED = "\033[31m";
    constexpr std::string_view GREEN = "\033[32m";
    constexpr std::string_view YELLOW = "\033[33m";
    constexpr std::string_view BLUE = "\033[34m";
    constexpr std::string_view MAGENTA = "\033[35m";
    constexpr std::string_view CYAN = "\033[36m";
    constexpr std::string_view WHITE = "\033[37m";

    constexpr std::string_view BRIGHT_RED = "\033[91m";
    constexpr std::string_view BRIGHT_GREEN = "\033[92m";
    constexpr std::string_view BRIGHT_YELLOW = "\033[93m";
    constexpr std::string_view BRIGHT_BLUE = "\033[94m";
    constexpr std::string_view BRIGHT_MAGENTA = "\033[95m";
    constexpr std::string_view BRIGHT_CYAN = "\033[96m";
    constexpr std::string_view BRIGHT_WHITE = "\033[97m";

    constexpr std::string_view COL_END = "\033[0m";

}

namespace characters
{
    constexpr char BACKSPACE = 8;
}

namespace util
{
#define ALWAYS_INLINE inline __attribute__((always_inline))
}
