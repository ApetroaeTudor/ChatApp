#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include "Server.h"
#include "ServerThreadManager.h"
#include <sys/wait.h>
#include <unistd.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <sys/types.h>
#include <ifaddrs.h>

using AtomicQueueString = atomic_queue::AtomicQueue2<std::string, constants::QUEUE_SIZE>;
using ServerManager = ServerThreadManager<AtomicQueueString>;

int gl_lan_dns_pid = -1;

bool kill_DNS_service();

void handle_sigint(int sig);

void correct_launch_instructions();

bool check_argv_1(const char *str, bool &local_only);

std::string get_local_ipv4();

int main(int argc, char *argv[]) {
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        utils::cerr_out_err("Failed to redirect sigint!\n");
        return EXIT_FAILURE;
    }

    std::string_view sv_addr = constants::LOCALHOST;
    bool local_only = true;

    const char *port_str = std::to_string(constants::PORT).c_str();

    if (argc == 2) {
        if (!check_argv_1(argv[1], local_only)) {
            correct_launch_instructions();
            return EXIT_FAILURE;
        }
    } else {
        correct_launch_instructions();
        return EXIT_FAILURE;
    }

    if (static_cast<int>(std::count(sv_addr.begin(), sv_addr.end(), '.') < 3)) {
        std::cerr << "Invalid IPv4 format\n";
        return EXIT_FAILURE;
    }

    int lan_dns_pid = -1;

    if (!local_only) {
        sv_addr = get_local_ipv4();
        lan_dns_pid = fork();
        if (lan_dns_pid < 0) {
            utils::cerr_out_err("Failed to launch the DNS process\n");
            return EXIT_FAILURE;
        } else if (lan_dns_pid == 0) // DNS process
        {
            execlp("avahi-publish", "avahi-publish", "-s", "ChatAppServer", "_ChatAppServer._tcp", port_str, nullptr);
            exit(1);
        }
        std::cout << "Started avahi DNS with pid: " << lan_dns_pid << std::endl;
        gl_lan_dns_pid = lan_dns_pid;
    }

    Server<AtomicQueueString, ServerManager> MAIN_SERVER(
        AF_INET,
        SOCK_STREAM,
        0,
        constants::PORT,
        sv_addr,
        SO_REUSEADDR | SO_REUSEPORT,
        1);
    MAIN_SERVER.listen_connections();
    MAIN_SERVER.launch_server_loop();

    if (!kill_DNS_service()) {
        return EXIT_FAILURE;
    }

    return 0;
}

bool kill_DNS_service() {
    if (gl_lan_dns_pid > 0) {
        if (kill(gl_lan_dns_pid, SIGTERM) < 0) {
            utils::cerr_out_err("Failed to terminate the DNS avahi process\n");
            return false;
            waitpid(gl_lan_dns_pid, nullptr, 0);
            std::cout << "Stopped DNS avahi process\n";
        }
    }
    return true;
}

void handle_sigint(int sig) {
    if (gl_lan_dns_pid > 0) {
        kill_DNS_service();
    }
    exit(1);
}

void correct_launch_instructions() {
    std::cerr << utils::get_color(colors::Colors::BRIGHT_RED) <<
            "Please launch the server like this:\n./Server LOCALHOST\nor\n./Server LAN\n" << std::endl;
}

bool check_argv_1(const char *str, bool &local_only) {
    bool local = !strcmp(str, "LOCALHOST\0");
    bool lan = !strcmp(str, "LAN\0");
    bool returnval = local || lan;
    local_only = local;
    return returnval;
}

std::string get_local_ipv4() {
    ifaddrs *ifaddr, *ifa;
    std::string local_ip;

    if (getifaddrs(&ifaddr) < 0) // returns a linked list of network interfaces
    {
        utils::cerr_out_err("Failed getifaddr\n");
        return {};
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) // iterates through the linked list
    {
        if (!ifa->ifa_addr) continue; // skip interfaces without ip

        if (ifa->ifa_addr->sa_family == AF_INET) // only ipv4
        {
            char addr[INET6_ADDRSTRLEN];
            void *sin_addr = &((sockaddr_in *) ifa->ifa_addr)->sin_addr;

            if (inet_ntop(AF_INET, sin_addr, addr,INET_ADDRSTRLEN)) {
                if (std::string(addr) != constants::LOCALHOST) {
                    local_ip = addr;
                    break;
                }
            }
        }
    }

    freeifaddrs(ifaddr);
    return local_ip;
}
