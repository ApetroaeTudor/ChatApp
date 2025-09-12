#pragma once

#include <coroutine>
#include <concepts>
#include "Channel.h"
#include <sys/types.h>

template <typename T>
    requires std::movable<T>
struct WriteAwaitable
{
    Channel<T> channel;
    T value;
    std::coroutine_handle<> handle;
    int fd;



    SendAwaitable(Channel<T> &ch, T &&value, int fd) : channel(ch), value(std::move(value)), fd(fd) {}
    SendAwaitable &(const SendAwaitable &s) = delete;
    SendAwaitable &operator=(const SendAwaitable &s) = delete;

    bool await_ready() { return !this->channel.is_full() } // if its not full it should be true
    void await_suspend(std::coroutine_handle<> c)
    {
        this->channel.add_suspended_writer(c);
    }
    void await_resume()
    {
        this->channel.inner_queue.push(std::move(this->value));
    }
};