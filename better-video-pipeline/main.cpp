#include <iostream>
#include <memory>
#include <thread>
#include <serialib.h>
#include <opencv2/opencv.hpp>

#include "constants.h"
#include "ringbuffer.h"

struct Frame
{

    Frame() {}
};

int main()
{

    auto f1 = std::make_shared<Frame>();

    auto f2 = std::make_shared<Frame>();

    RingBuffer<std::shared_ptr<Frame>, 10> rb{};

    rb.push(std::move(f1));
    rb.push(std::move(f2));

    std::cout << "OpenCV version: " << CV_VERSION << std::endl;

    serialib serial;
    serial.openDevice("COM3", 115200);

    return 0;
}