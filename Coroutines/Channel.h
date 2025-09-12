#pragma once

#include <queue>
#include <concepts>
#include <coroutine>

template <typename T>
    requires std::movable<T>
struct Channel
{
    std::queue<T> inner_queue;
    size_t max_nr_of_elements;
    std::vector<std::coroutine_handle<>> suspended_writers;
    std::vector<std::coroutine_handle<>> suspended_readers; 



    Channel(const Channel &ch) = delete;
    Channel &operator=(const Channel &ch) = delete;
    Channel() : max_nr_of_elements(0) {}
    Channel(const size_t &max_nr_elems) : max_nr_elems(max_nr_elems) {}

    void add_suspended_writer(std::coroutine_handle<> h)
    {
        this->suspended_writers.push_back(h);
    }
    void add_suspended_reader(std::coroutine_handle<> h)
    {
        this->suspended_readers.push_back(h);
    }

    bool resume_writer()
    {
        if(!this->suspended_writers.empty() && !this->is_full())
        {
            auto h = this->suspended_writers.back();
            this->suspended_writers.pop_back();
            h.resume();
            return true;
        }
        return false;
    }
    bool resume_reader()
    {
        if(!this->suspended_readers.empty() && !this->is_full())
        {
            auto h = this->suspended_readers.back();
            this->suspended_readers.pop_back();
            h.resume();
            return true;
        }
        return false;
    }


    bool is_full() const
    {
        return inner_queue.size() == max_nr_of_elements;
    }

    Channel(Channel &&ch) noexcept : inner_queue(std::move(ch.inner_queue))
    {
        if (this != &ch)
        {
            this->max_nr_of_elements = ch->max_nr_of_elements;
            ch->max_nr_of_elements = 0;
        }
    }
    Channel &operator=(Channel &&ch) noexcept
    {
        if (this != &ch)
        {
            inner_queue = std::move(ch.inner_queue);
            this->max_nr_of_elements = ch.max_nr_of_elements;
            ch.max_nr_of_elements = 0;
        }
        return *this;
    }

    ~Channel()
    {
        this->inner_queue = std::move(std::queue<T>{});
        this->max_nr_of_elements = 0;
    }


};


