#include "Epoll.h"

EpollManager::EpollManager()
{
    epfd = epoll_create1(0);
    if (epfd < 0)
    {
        throw std::runtime_error("Failed to initialize epfd\n");
    }
}

EpollManager::EpollManager(const EpollManager &other)
{
    this->operator=(other);
}

EpollManager &EpollManager::operator=(const EpollManager &other)
{
    if (this != &other)
    {
        epfd = other.epfd;
        events = other.events;
    }
    return *this;
}

EpollManager &EpollManager::operator=(EpollManager &&other) noexcept
{
    if (this != &other)
    {
        epfd = other.epfd;
        other.epfd = -1;
        this->events = std::move(other.events);
    }
    return *this;
}

EpollManager::EpollManager(EpollManager &&other) noexcept
{
    this->operator=(std::move(other));
}

EpollManager::~EpollManager()
{
    close(epfd);
}

void EpollManager::add_monitored_fd(const EpollPair &ep)
{

    if (epfd > 0)
    {
        epoll_event ev{};
        ev.data.fd = ep.fd;
        ev.events = ep.event;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, ep.fd, &ev) < 0)
        {
            throw std::runtime_error("Failed to initialize epoll ctl\n");
        }
    }
    else
    {
        throw std::runtime_error("EPFD not initialized\n");
    }
}

int EpollManager::wait_events(int timeout)
{
    if (epfd >= 0)
    {
        int ret = -1;
        ret = epoll_wait(epfd, this->events.data(), events.size(), timeout);
        if (ret < 0)
        {
            throw std::runtime_error("Failed to do epoll_wait\n");
        }
        return ret;
    }
    {
        throw std::runtime_error("EPFD not initialized\n");
    }
}

bool EpollManager::check_event(int fd, unsigned int event_type, int events_nr)
{
    if (epfd < 0)
    {
        throw std::runtime_error("EPFD not initialized\n");
    }
    if (fd < 0)
    {
        throw std::invalid_argument("Bad passed FD\n");
    }

    if (events_nr < 0)
    {
        throw std::invalid_argument("Bad event nr passed\n");
    }
    if (events_nr == 0)
    {
        return false;
    }

    return std::any_of(this->events.begin(), this->events.begin() + events_nr,
                       [&fd, &event_type](const epoll_event &ev)
                       {
                           return (ev.data.fd == fd) && (ev.events & event_type);
                       });
}

void EpollManager::clear_events()
{
    this->events.fill({});
}