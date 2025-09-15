#include "resources.h"
#include <thread>
#include <iostream>

#include "Client.h"




int main(int argc, char *argv[])
{

    std::string_view sv_addr = constants::SV_ADDR;
    if (argc == 2)
    {
        sv_addr = std::string_view(argv[1]);
    }
    else if (argc > 2)
    {
        std::cerr << "Correct call: ./Server <OPTIONAL:sv_addr>, if not specified then 127.0.0.1 is used\n";
        return EXIT_FAILURE;
    }

    if (static_cast<int>(std::count(sv_addr.begin(), sv_addr.end(), '.') < 3))
    {
        std::cerr << "Invalid IPv4 format\n";
        return EXIT_FAILURE;
    }

    try
    {
        Client cl(AF_INET, SOCK_STREAM, 0, sv_addr.data(), constants::PORT);
        cl.make_cl_socket_nonblocking();
        std::jthread send_jthr([&]()
                               { cl.start_send_loop(); });
        std::jthread recv_jthr([&]()
                               { cl.start_receive_loop(); });
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}

