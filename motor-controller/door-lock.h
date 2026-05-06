#pragma once

#include "device.h"
#include "signal.h"

struct DoorLock : Device
{
    DoorLock() = default;

    void processSignal(Signal &signal)
    {
    }

    void tick() override
    {
        Serial.print("ticking");
    }
};