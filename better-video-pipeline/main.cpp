#include <iostream>
#include <memory>
#include <thread>
#include <serialib.h>
#include <opencv2/opencv.hpp>
#include <chrono>

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

    void wait()
    {
        if (t.joinable())
            t.join();
    }

private:
    virtual void run() = 0;
};

template <LogLevel L = LogLevel::INFO>
struct CameraStreamer : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>
{
    Logger<L> logger{};
    cv::VideoCapture cap{config::CAMERA_INPUT, config::CAMERA_BACKEND};

    CameraStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer) : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{buffer}
    {
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
        cap.set(cv::CAP_PROP_FPS, 30);

        double fps = cap.get(cv::CAP_PROP_FPS);
        double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
        double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        double fourcc = cap.get(cv::CAP_PROP_FOURCC);

        logger.info("FPS: " + std::to_string(fps));
        logger.info("Width: " + std::to_string(width));
        logger.info("Height: " + std::to_string(height));
        auto countdown = std::vector{3, 2, 1};
        std::ranges::for_each(countdown, [this](auto &v)
                              { 
                                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                                    logger.info(v); });
    }

    void run() override
    {
        if (!cap.isOpened())
        {
            logger.error("camera is in use by another process");
            return;
        }
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
            logger.info("pushing frame");
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

    std::cout << "OpenCV version: " << CV_VERSION << std::endl;

    serialib serial;
    auto res = serial.openDevice(config::COMPORT, config::BAUDRATE);

    Logger<DEBUG> log{};

    log.info("res is: " + res);

    log.debug("hello world");

    log.warn("hello world");

    log.info("hello world");

    log.error("hello world");

    auto rb = std::make_shared<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>>();

    auto cameraStreamer = CameraStreamer<config::LOG_LEVEL>(rb);
    // cameraStreamer->wait();
    cameraStreamer.start();

    return 0;
}