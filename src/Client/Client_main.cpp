#include "resources.h"
#include <thread>
#include <iostream>


#include "Client.h"




int main(int argc, char *argv[])
{
    std::string_view LOCALHOST = constants::LOCALHOST;
    if (argc == 2)
    {
        LOCALHOST = std::string_view(argv[1]);
    }
    else if (argc > 2)
    {
        utils::cerr_out_err("Correct call: ./Server <OPTIONAL:LOCALHOST>, if not specified then 127.0.0.1 is used\n");
        return EXIT_FAILURE;
    }

    if (static_cast<int>(std::count(LOCALHOST.begin(), LOCALHOST.end(), '.') < 3))
    {
        utils::cerr_out_err("Invalid IPv4 format\n");
        return EXIT_FAILURE;
    }

    try
    {
        Client cl(AF_INET, SOCK_STREAM, 0, LOCALHOST.data(), constants::PORT);
        cl.make_cl_socket_nonblocking();
        std::jthread send_jthr([&]()
                               { cl.start_send_loop(); });
        std::jthread recv_jthr([&]()
                               { cl.start_receive_loop(); });
    }
    catch (const std::exception &e)
    {
        utils::cerr_out_err(e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

