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
struct RingBuffer<T, N>
{
    std::array<T, N> data{};

    int start{};
    int end{};

    RingBuffer()
    {
    }
};