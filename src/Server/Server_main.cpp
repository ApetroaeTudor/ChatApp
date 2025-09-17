#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include "Server.h"
#include "JThreadDispatcher.h"

using AtomicQueueString = atomic_queue::AtomicQueue2<std::string, constants::QUEUE_SIZE>;
using ServerThreadManager = ServerManager<AtomicQueueString>;

int main(int argc, char *argv[]) {
    std::string_view sv_addr = constants::SV_ADDR;
    if (argc == 2) {
        sv_addr = std::string_view(argv[1]);
    } else if (argc > 2) {
        std::cerr << "Correct call: ./Server <OPTIONAL:sv_addr>, if not specified then INADDR_ANY is used\n";
        return EXIT_FAILURE;
    }

    if (static_cast<int>(std::count(sv_addr.begin(), sv_addr.end(), '.') < 3)) {
        std::cerr << "Invalid IPv4 format\n";
        return EXIT_FAILURE;
    }

    Server<AtomicQueueString, ServerThreadManager> MAIN_SERVER(
        AF_INET,
        SOCK_STREAM,
        0,
        constants::PORT,
        SO_REUSEADDR | SO_REUSEPORT,
        1
    );
    MAIN_SERVER.listen_connections();
    MAIN_SERVER.launch_server_loop();

    return 0;
}
