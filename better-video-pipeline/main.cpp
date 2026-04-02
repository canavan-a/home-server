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

    Streamer(std::shared_ptr<RingBuffer<T, N>> buf) : buffer{buf}
    {
    }

    void start()
    {
        t = std::thread([this]()
                        { this->run(); });
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

template <LogLevel L = LogLevel::INFO>
struct CameraStreamer : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>
{
    Logger<L> logger{};
    cv::VideoCapture cap{env::CAMERA_INPUT};

    CameraStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer) : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{buffer}
    {
    }

    void run() override
    {
        std::cout << "hello world I am a Camera streamer" << nl;
        auto count{0};
        for (;;)
        {
            std::cout << ++count << nl;
            cv::Mat frame;
            cap >> frame;
            if (frame.empty())
            {
                logger.debug("captured empty frame");
                continue;
            }
            this->buffer->push(frame);
        }
    }

    ~CameraStreamer()
    {
        if (t.joinable())
            t.join();

        cap.release();
    }
};

int main()
{

    auto f1 = std::make_shared<Frame>();

    auto f2 = std::make_shared<Frame>();

    RingBuffer<std::shared_ptr<Frame>, 10> buffy{};

    buffy.push(std::move(f1));
    buffy.push(std::move(f2));

    std::cout << "OpenCV version: " << CV_VERSION << std::endl;

    serialib serial;
    auto res = serial.openDevice(env::COMPORT, env::BAUDRATE);

    Logger<DEBUG> log{};

    log.info("res is: " + res);

    log.debug("hello world");

    log.warn("hello world");

    log.info("hello world");

    log.error("hello world");

    auto rb = std::make_shared<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>>();

    auto cameraStreamer = CameraStreamer<config::LOG_LEVEL>(rb);

    return 0;
}