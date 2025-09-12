#pragma once

namespace constants
{
    constexpr int MAX_LEN         = 200;
    constexpr int PORT            = 8080;
    constexpr auto SV_ADDR        = "127.0.0.1";
}

namespace colors
{
    constexpr auto RED            = "\033[31m";
    constexpr auto GREEN          = "\033[32m";
    constexpr auto YELLOW         = "\033[33m";
    constexpr auto BLUE           = "\033[34m";
    constexpr auto MAGENTA        = "\033[35m";
    constexpr auto CYAN           = "\033[36m";

    constexpr auto BRIGHT_RED     = "\033[91m";
    constexpr auto BRIGHT_GREEN   = "\033[92m";
    constexpr auto BRIGHT_YELLOW  = "\033[93m";
    constexpr auto BRIGHT_MAGENTA = "\033[94m";
    constexpr auto BRIGHT_CYAN    = "\033[95m";

    constexpr auto COL_END        =  "\033[0m";
}

namespace characters
{
    constexpr char BACKSPACE = 8;
}


