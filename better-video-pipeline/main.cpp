#include <iostream>
#include <memory>
#include <thread>
#include <serialib.h>
#include <opencv2/opencv.hpp>

#include "constants.h"
#include "ringbuffer.h"
#include "config.h"
#include "logger.h"

#define nl "\n"

struct Frame
{

    Frame() {}
};

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

template <typename T, size_t N>
struct Streamer
{

    std::shared_ptr<RingBuffer<T, N>> buffer;
    std::thread t;

    Streamer(std::shared_ptr<RingBuffer<T, N>> buf) : buffer{buf}, t([this]()
                                                                     { 
                                                                        this->run();
                                                                         return; })
    {
    }

    virtual ~Streamer()
    {
        if (t.joinable())
        {
            t.join();
        }
    }

private:
    virtual void run() = 0;
};

struct CameraStreamer : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>
{
    CameraStreamer() : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{} {}

    void run() override
    {

        std::cout << "hello world I am a Camera streamer" << nl;
    }
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
    auto res = serial.openDevice("COM3", 115200);

    Logger<DEBUG> log{};

    log.info("res is: " + res);

    log.debug("hello world");

    log.warn("hello world");

    log.info("hello world");

    log.error("hello world");

    return 0;
}