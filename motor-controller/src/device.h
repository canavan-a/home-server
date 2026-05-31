#pragma once

struct Device
{
    Device() = default;

    virtual void tick() = 0;
};