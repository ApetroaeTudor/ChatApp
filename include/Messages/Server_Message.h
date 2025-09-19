#pragma once

#include <string>
#include <sstream>
#include <numeric>

constexpr char split_token = '`';
constexpr int nr_of_separations = 8;

struct Server_Message {
    std::string thread_number{};
    std::string action{};
    std::string fd{};
    std::string id{};
    std::string name{};
    std::string color{};
    std::string init_done{};
    std::string msg_content{};

    std::string get_concatenated_msg() {
        return thread_number + split_token + action + split_token + fd + split_token + id + split_token + name +
               split_token + color + split_token + init_done + split_token + msg_content + split_token;
    }

    void init_all_fields(const char* thr_nr, const char* action, const char* fd, const char* id, const char* name, const char* color, const char* init_done,const char* msg_content) {
        this->thread_number = thr_nr;
        this->action = action;
        this->fd = fd;
        this->id = id;
        this->name = name;
        this->color = color;
        this->init_done = init_done;
        this->msg_content = msg_content;
    }



    void construct_from_string(const std::string &str) {
        if (std::accumulate(str.begin(), str.end(), 0, [](int a, char b) {
            return (b == split_token) ? (a + 1) : a;
        }) != nr_of_separations) {
            throw std::runtime_error("Error in message construction. Invalid Msg format\n");
        }
        std::istringstream iss(str);
        std::getline(iss,thread_number,split_token);
        std::getline(iss,action,split_token);
        std::getline(iss,fd,split_token);
        std::getline(iss,id,split_token);
        std::getline(iss,name,split_token);
        std::getline(iss,color,split_token);
        std::getline(iss,init_done,split_token);
        std::getline(iss,msg_content,split_token);
    }
};
