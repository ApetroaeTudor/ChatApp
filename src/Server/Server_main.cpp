#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include "Server.h"

int main(int argc, char* argv[])
{


    std::string_view sv_addr = constants::SV_ADDR;
    if (argc == 2)
    {
        sv_addr = std::string_view(argv[1]);
    }
    else if (argc > 2)
    {
        std::cerr << "Correct call: ./Server <OPTIONAL:sv_addr>, if not specified then INADDR_ANY is used\n";
        return EXIT_FAILURE;
    }

    if (static_cast<int>(std::count(sv_addr.begin(), sv_addr.end(), '.') < 3))
    {
        std::cerr << "Invalid IPv4 format\n";
        return EXIT_FAILURE;
    }



    int sv_socket;
    int cl_socket;


    struct sockaddr_in s_addr;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr.sin_port = htons(constants::PORT);
    s_addr.sin_family = AF_INET;

    sv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (sv_socket < 0)
    {
        perror("Failed to open socket\n");
        exit(EXIT_FAILURE);
    }

    int opt = 1;

    if (setsockopt(sv_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        perror("Failed to setsockopt\n");
        exit(EXIT_FAILURE);
    }

    if (bind(sv_socket, reinterpret_cast<sockaddr *>(&s_addr), sizeof(s_addr)) < 0)
    {
        perror("Failed to bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sv_socket, 3) < 0)
    {
        perror("Failed to listen\n");
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(s_addr);
    cl_socket = accept(sv_socket, reinterpret_cast<sockaddr *>(&s_addr), &addr_len);
    if (cl_socket < 0)
    {
        perror("Fail on accept\n");
        exit(EXIT_FAILURE);
    }

    char buffer[constants::MAX_LEN];
    if (recv(cl_socket, &buffer, sizeof(buffer), 0) < 0)
    {
        perror("failed to receive msg\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(buffer, "hi\0") == 0)
    {
        if (send(cl_socket, "hello\n", 6, 0) < 0)
        {
            perror("failed to send msg\n");
            exit(EXIT_FAILURE);
        }
    }

    std::cout << colors::RED << "MSG RECEIVED: " << buffer << colors::COL_END << std::endl;

    close(sv_socket);
    close(cl_socket);

    return 0;
}