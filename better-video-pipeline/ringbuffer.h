#pragma once

#include <iostream>
#include <expected>
#include <memory>
#include <array>
#include <semaphore>
#include <ranges>
#include <mutex>

#define nl "\n"

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

template <typename T, size_t N>
struct RingBuffer
{
    std::array<T, N> data{};
    std::mutex mtx{};
    std::condition_variable signal{};

    int start{};
    int end{};

    int count{};

    RingBuffer()
    {
    }

    void push(const T &item)
    {
        signal.notify_all();
        std::lock_guard<std::mutex> lock(this->mtx);

        this->data[end] = item;

        if ((end) == N - 1)
        {
            end = 0;
        }
        else
        {
            ++end;
        }

        if (end == start)
        {
            if ((start) == N - 1)
            {
                start = 0;
            }
            else
            {
                ++start;
            }
        }

        count = std::min((int)N, count + 1);
    }

    Result<T> pop()
    {
        std::lock_guard<std::mutex> lock(this->mtx);

        if (this->isEmpty())
            return Err{"buffer is empty"};

        T item = data[start];
        if (start == N - 1)
        {
            start = 0;
        }
        else
        {
            ++start;
        }
        --count;
        return item;
    }

    Result<T> peek()
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        if (this->isEmpty())
            return Err{"buffer is empty"};

        T item = data[start];

        return item;
    }

    Result<T> peekFront()
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        if (this->isEmpty())
            return Err{"buffer is empty"};
        T item = data[end == 0 ? N - 1 : end - 1];
        return item;
    }

private:
    bool isEmpty()
    {
        return count == 0;
    }
};