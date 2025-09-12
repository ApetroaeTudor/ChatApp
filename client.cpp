#include <iostream>
#include "resources.h"
#include <thread>
#include <queue>

int main()
{
    std::cout<<colors::BRIGHT_RED<<"hey\n"<<colors::COL_END;
    std::cout<<std::thread::hardware_concurrency();

    return 0;
}