#pragma once

#include <iostream>
#include <expected>
#include <memory>
#include <array>
#include <ranges>

#define nl "\n"

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

template <typename T, size_t N>
struct RingBuffer
{
    std::array<T, N> data{};

    int start{};
    int end{};

    int count{};

    RingBuffer()
    {
    }

    void push(const T &item)
    {
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
    bool isEmpty()
    {
        return count == 0;
    }

    Result<T> pop()
    {
        if (this->isEmpty())
        {
            return Err{"buffer is empty"};
        }

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
};