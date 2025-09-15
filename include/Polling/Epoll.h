#pragma once
#include <sys/epoll.h>
#include <array>
#include <stdexcept>
#include <string.h>
#include <unistd.h>
#include "resources.h"

struct EpollPair
{
    int fd;
    int event;
};

class EpollManager
{
private:
    int epfd = -1;
    std::array<epoll_event,constants::MAX_EVENTS> events;


public:
    EpollManager();
    
    EpollManager(const EpollManager &other);
    EpollManager &operator=(const EpollManager &other);

    EpollManager& operator=(EpollManager&& other) noexcept;
   
    EpollManager(EpollManager &&other) noexcept;

    ~EpollManager();

    void add_monitored_fd(const EpollPair& ep);

    int wait_events(int timeout = 0);

    bool check_event(int fd,unsigned int event_type, int event_nr);

    void clear_events();
    
};