#include "resources.h"
#include <thread>
#include <iostream>
#include <memory>


#include "Client.h"


void correct_launch_instructions();
bool check_argv_1(const char *str, bool &local_only);
std::string get_sv_addr();

int main(int argc, char *argv[])
{
    std::string addr_to_connect = constants::LOCALHOST.data();
    bool local_only = true;
    if (argc == 2)
    {
        if(!check_argv_1(argv[1],local_only))
        {
            correct_launch_instructions();
            return EXIT_FAILURE;
        }
        
    }
    else
    {
        correct_launch_instructions();
        return EXIT_FAILURE;
    }

    if(!local_only)
    {
        addr_to_connect = get_sv_addr();
        if(addr_to_connect.empty())
        {
            utils::cerr_out_err("Server isn't correcty initialized! Failed connect\n");
        }
    }

    if (static_cast<int>(std::count(addr_to_connect.begin(), addr_to_connect.end(), '.') < 3))
    {
        utils::cerr_out_err("Invalid IPv4 format\n");
        return EXIT_FAILURE;
    }

    try
    {
        Client cl(AF_INET, SOCK_STREAM, 0, addr_to_connect, constants::PORT);
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


void correct_launch_instructions()
{
    std::cerr << utils::get_color(colors::Colors::BRIGHT_RED) << "Please launch the client like this:\n./Client LOCALHOST\nor\n./Client LAN\n"<<std::endl;
}

bool check_argv_1(const char *str, bool &local_only)
{
    bool local = !strcmp(str, "LOCALHOST\0");
    bool lan = !strcmp(str, "LAN\0");
    bool returnval = local || lan;
    local_only = local;
    return returnval;
}

std::string get_sv_addr()
{
    std::string cmd = "avahi-browse -r -t _ChatAppServer._tcp | awk -F'[][]' '/address/ && $2 ~ /^192\\.168\\./ {print $2}'";
    std::unique_ptr<FILE,decltype(&pclose)> pipe(popen(cmd.c_str(),"r"),pclose);
    if(!pipe)
    {
        return {};
    }
    std::string result;
    std::array<char,128> buffer;
    while(fgets(buffer.data(),buffer.size(),pipe.get())!=nullptr)
    {
        result += buffer.data();
    }
    if(!result.empty() && result.back() == '\n')
    {
        result.pop_back();
    }
    return result;
}


