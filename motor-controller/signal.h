#pragma once

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

enum Action
{
    None,
    ManualControl,
    ObjectDetection,
    Config,
    Mode,
    Lock,
};