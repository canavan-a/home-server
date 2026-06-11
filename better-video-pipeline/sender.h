#pragma once

#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <cerrno>
#include <cstring>
#include <fcntl.h>

#include <serialib.h>

#include "coco.h"
#include "config.h"
#include "ringbuffer.h"
#include "inferenceutil.h"
#include "logger.h"
#include "action.h"

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

namespace special
{
    template <typename T>
    struct AtomicQueue
    {

        std::queue<T> queue{};

        std::mutex mutex;

        AtomicQueue() = default;

        void push(T item)
        {
            std::unique_lock<std::mutex> lock(mutex);

            queue.push(item);
        }

        Result<T> pop()
        {

            std::unique_lock<std::mutex> lock(mutex);
            if (queue.empty())
                return Err{"queue is empty"};
            T item = queue.front();
            queue.pop();
            return item;
        }

        bool empty()
        {
            std::unique_lock<std::mutex> lock(mutex);
            return queue.empty();
        }
    };
}

struct SerialControl
{
    Action action;

    int speed{config::DEFAULT_MAX_SPEED};

    int home_x{config::HOME_X};

    int home_y{config::HOME_Y};

    int pos_x{config::HOME_X};

    int pos_y{config::HOME_Y};

    SerialControl(Action myAction) : action{myAction} {};

    std::string marshall()
    {

        std::string marshalledAction{std::to_string(static_cast<int>(action))};

        switch (action)
        {
        // case Action::ConfigSpeed:
        //     marshalledAction = marshalledAction + " " + std::to_string(speed);
        //     break;

        case Action::SET_HOME:
            marshalledAction = marshalledAction + " " + std::to_string(home_x) + " " + std::to_string(home_y);
            break;
		case Action::GO_TO_POSITION:
			marshalledAction = marshalledAction + " " + std::to_string(pos_x) + " " + std::to_string(pos_y);
			break;
        default:
        	marshalledAction = marshalledAction;
            break;
        }

        marshalledAction += " \r\n";

        return marshalledAction;
    }
};

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

    // Controls section
    serialib serial;

    std::shared_ptr<special::AtomicQueue<SerialControl>> controlQueue{};

    SerialSender() : sendBuffer{std::make_shared<RingBuffer<InferenceObjects::DetectedObject, 4>>()},
                     controlQueue{std::make_shared<special::AtomicQueue<SerialControl>>()}
    {

        this->openSerial();
    };

    void openSerial()
    {
        this->safeCloseSerial();
        auto res = serial.openDevice(config::COMPORT, config::BAUDRATE);
        if (res != 1)
            logger.error("could not open serial");
    }

    void safeCloseSerial()
    {
        if (serial.isDeviceOpen())
        {
            serial.closeDevice();
        }
    }

    void enqueueControl(SerialControl control)
    {
        controlQueue->push(control);
        cv.notify_one();
    }

    void gatherObjects(std::vector<InferenceObjects::DetectedObject> &objects)
    {
        for (auto &object : objects)
        {
            if (object.type == detectedClass)
            {
                sendBuffer->push(object);
                cv.notify_one();
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
                                cv.wait(lock, [this]{ return !controlQueue->empty() || sendBuffer->hasData(); });
                                lock.unlock();

                                std::string msg{};

                                if (auto control = controlQueue->pop())
                                {
                                    msg = control->marshall();
                                }
                                else if (auto value = sendBuffer->pop())
                                {
                                    msg = value->marshall();
                                }
                                else
                                {
                                    continue;
                                }

                                logger.info("serial send: " + msg);
                                auto res = this->serial.writeString(msg.c_str());
                                if (res != 1){
                                    logger.error("could not send data on serial: " + std::string(strerror(errno)) + " — attempting to reopen serial com");
                                    this->openSerial();
                                }

                            } });
    }

    ~SerialSender()
    {

        if (t.joinable())
            t.join();

        this->safeCloseSerial();
    }
};
