#include <iostream>
#include <memory>
#include <thread>
#include <serialib.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <semaphore>
#include <mutex>

// coral and tflite imports
#include "coral/detection/adapter.h"
#include "coral/tflite_utils.h"

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
    std::shared_ptr<std::binary_semaphore> cameraStreamReady{};
    Logger<L> logger{};
    cv::VideoCapture cap{config::CAMERA_INPUT, config::CAMERA_BACKEND};

    CameraStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer) : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{buffer},
                                                                                                    std::make_shared<std::binary_semaphore>{}
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
                                std::this_thread::sleep_for(std::chrono::milliseconds(800));
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
        cameraStreamReady->release();
        for (;;)
        {
            std::cout << ++count << nl;
            cv::Mat frame;
            cap >> frame;
            if (frame.empty())
            {
                logger.warn("captured empty frame");
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

template <LogLevel L = LogLevel::INFO>
struct InferenceConsumer : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>
{

    std::shared_ptr<std::binary_semaphore> cameraStreamReady{};
    Logger<L> logger{};
    // inference result buffer or eat the io overhead..... ??

    std::unique_ptr<tflite::FlatFufferModel> model;
    std::shared_ptr<edgetpu::EdgeTpuContext> tpu_context;
    std::unique_ptr<tflite::Interpreter> interpreter;

    InferenceConsumer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer, std::shared_ptr<std::binary_semaphore> csReady) : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{buffer}, cameraStreamReady{csReady}
    {
        model = coral::LoadModelOrDie(config::OBJECT_DETECTION_MODEL_PATH);
        tpu_context = coral::GetEdgeTpuContextOrDie("usb");
        if (!tpu_context)
        {
            logger.error("could not get edgetpu context ");
            std::exit();
        }
        interpreter = coral::MakeEdgeTpuInterpreterOrDie(*model, tpu_context.get());
        interpreter->AllocateTensors();
        logger.info("inference consumer constructed");
    }

    void run() override
    {
        logger.info("waiting for camera stream to start");
        cameraStreamReady->acquire();
        logger.info("started inference consumer process");
        for (;;)
        {

            Result<cv::Mat> peekValue = this->buffer->peek();

            if (!peekValue)
            {
                logger.debug("empty frame buffer");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                logger.debug("waited on empty frame");
                continue;
            }
            cv::Mat frame{peekValue.value()};
            logger.info("resizing frame");
            cv::Mat resized;
            cv::resize(frame, resized, cv::Size(320, 320));
            cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
            logger.info("creating input buffer");
            auto *input = interpreter->typed_input_tensor<uint8_t>(0);
            std::memcpy(input, resized.data, resized.total() * resized.elemSize());

            logger.info("invoking request");
            interpreter->Invoke();

            auto results = coral::GetDetectionResults(*interpreter, 0.5f);
            for (auto &obj : results)
            {
                logger.info("detected id=" + std::to_string(obj.id) + " score=" + std::to_string(obj.score));
            }

            logger.debug("running inference for frame");
        }
    }

    ~InferenceConsumer()
    {
        if (t.joinable())
            t.join();
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

    auto cameraStreamer = CameraStreamer<LogLevel::WARN>(rb);
    // cameraStreamer->wait();
    cameraStreamer.start();

    auto inferenceStreamer = InferenceConsumer(rb, cameraStreamer.cameraStreamReady);

    inferenceStreamer.start();

    return 0;
}