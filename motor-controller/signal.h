#pragma once

#include "action.h"

struct SignalProcessor
{
    SignalProcessor() = default;

    Signal processLine(std::string &line)
    {
    }
};

struct Signal
{

    Action action;

    Signal() = default;
};
