#include <iostream>
#include <memory>
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

    return 0;
}