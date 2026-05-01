#pragma once

#include <memory>
#include <thread>

#include <serialib.h>

#include "coco.h"
#include "config.h"
#include "ringbuffer.h"
#include "inferenceutil.h"
#include "logger.h"

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

template <LogLevel L = LogLevel::INFO>
struct SerialSender
{
    Logger<L> logger{};
    std::string comPort{config::COMPORT};

    COCO detectedClass{COCO::PERSON};

    std::shared_ptr<RingBuffer<InferenceObjects::DetectedObject, 4>> sendBuffer{};

    std::thread t;

    std::mutex mtx;
    std::condition_variable cv;

    SerialSender() : sendBuffer{std::make_shared<RingBuffer<InferenceObjects::DetectedObject, 4>>()} {};

    void gatherObjects(std::vector<InferenceObjects::DetectedObject> &objects)
    {

        for (auto &object : objects)
        {
            if (object.type == detectedClass)
            {
                sendBuffer->push(object);
                cv.notify_one();
                // signal here
            }
        }
    }

    void start()
    {
        t = std::thread([this]()
                        {
                            for (;;)
                            {
                                std::unique_lock<std::mutex> lock(mtx);
                                cv.wait(lock);
                                auto value = sendBuffer->peekFront();
                                if (!value){
                                    continue;
                                }
                                // use the dereference operator *value

                            } });
    }

    ~SerialSender()
    {
        if (t.joinable())
            t.join();
    }
};
