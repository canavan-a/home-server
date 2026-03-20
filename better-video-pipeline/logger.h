#pragma once

#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>

enum LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    NONE
};

template <LogLevel L>
struct Logger
{
    Logger() = default;

    template <typename T>
    void debug(T value)
    {
        if (L >= LogLevel::DEBUG)
        {
            std::cout << "DEBUG: " << value << std::endl;
        }
    }

    template <typename T>
    void info(T value)
    {
        if (L <= LogLevel::INFO)
            std::cout << "\033[32m" << "INFO: " << value << "\033[0m" << std::endl;
    }

    template <typename T>
    void warn(T value)
    {
        if (L <= LogLevel::WARN)
            std::cout << "\033[33m" << "WARN: " << value << "\033[0m" << std::endl;
    }

    template <typename T>
    void error(T value)
    {
        if (L <= LogLevel::ERROR)
            std::cout << "\033[31m" << "ERROR: " << value << "\033[0m" << std::endl;
    }
};
