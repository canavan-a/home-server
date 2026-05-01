#pragma once

#include <opencv2/opencv.hpp>
#include <array>
#include <ranges>
#include <atomic>
#include <numeric>
#include <ctime>
#include <thread>
#include <vector>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <filesystem>

#include "inferenceutil.h"
#include "config.h"
#include "ringbuffer.h"
#include "logger.h"

constexpr int PRECLIP_QUEUE_SIZE{100};
constexpr int CLIP_START_THRESH{20}; // frames to start the clip
constexpr int CLIP_STALE_THRESH{35}; // frames to stop the clip

constexpr int CLIP_FPS{20};

template <LogLevel L = LogLevel::INFO>
struct ClipHandler
{

    Logger<L> logger{};
    RingBuffer<cv::Mat, PRECLIP_QUEUE_SIZE> preclip{};

    const std::array<COCO, 1> criticalObject{COCO::PERSON};

    std::atomic<bool> clipping{false};
    int startCounter{0};
    int staleCounter{0};

    int quality{50};

    std::mutex mtx;
    std::condition_variable cond;
    bool frameReady{false};

    std::shared_ptr<std::atomic<bool>> clipStopped{};

    std::vector<cv::Mat> currentClip{};

    cv::Mat shareFrame;

    RingBuffer<float, 150> frameRates{};

    ClipHandler()
    {
        std::filesystem::create_directories(config::clipDirName);
        std::filesystem::create_directories(std::string(config::clipDirName) + "-tmp");
    };

    // objects are deemed to already be filtered for confidence
    void handleInferenceFrame(cv::Mat frame, const std::vector<InferenceObjects::DetectedObject> &confidentObjects, float frameRate)
    {

        frameRates.push(frameRate);

        // add preclip
        if (this->containsCriticalObject(confidentObjects))
        {
            handleDetection(frame);
        }
        else
        {
            handleNoDetection(frame);
        }

        // always push frames to either preclip or current clip buffers
        if (clipping)
        {
            {
                std::unique_lock<std::mutex> lock(mtx);
                shareFrame = frame;
                frameReady = true;
            }
            cond.notify_one();
        }
        else
        {
            //
            preclip.push(frame);
        }
    }

    void handleDetection(cv::Mat frame)
    {
        if (!clipping)
        {
            if (startCounter >= CLIP_START_THRESH)
            {
                startClip();
            }
            else
            {
                incrementStartCount();
            }
        }
        else
        {
            staleCounter = 0;
            // clipping
        }
    }

    void handleNoDetection(cv::Mat frame)
    {
        if (!clipping)
        {
            startCounter = 0;
            //  not clipping, no object, pass
            return;
        }
        else
        {
            if (staleCounter >= CLIP_STALE_THRESH)
            {
                endClip();
            }
            else
            {
                incrementStaleCount();
            }
        }
    }

    void startClip()
    {
        logger.info("starting clip (normal)");
        startClipProcess();
        clipping = true;
    }

    void endClip()
    {
        logger.info("clip complete, ending clip");
        resetCounters();
        clipping = false;
        if (clipStopped)
            clipStopped->store(true);
        cond.notify_one();
    }

    void startClipProcess()
    {

        logger.info("starting clip process thread");
        auto pc = preclip.dump();

        auto frames = frameRates.dump();
        auto rate = frames.empty() ? 20.0f : std::accumulate(frames.begin(), frames.end(), 0.0f) / frames.size();

        clipStopped = std::make_shared<std::atomic<bool>>(false);

        auto t = std::thread([this, pre = std::move(pc), rate, stopped = clipStopped]()
                             {


            auto timestamp = std::to_string(std::time(nullptr));
            std::string tmpPath = config::clipDirName + "-tmp/" + timestamp + ".mp4";
            std::string finalPath = config::clipDirName + "/" + timestamp + ".mp4";
            cv::VideoWriter writer(
              tmpPath,
              cv::VideoWriter::fourcc('m','p','4','v'),
              rate,
              cv::Size(pre[0].cols, pre[0].rows)
            );

            writer.set(cv::VIDEOWRITER_PROP_QUALITY, quality);
            for (auto &fr : pre)
                writer.write(fr);

            for (;;){
                std::unique_lock<std::mutex> lock(mtx);
                cond.wait(lock, [&stopped, this]{ return frameReady || stopped->load(); });
                if (stopped->load())
                    break;
                auto frame = shareFrame.clone();
                frameReady = false;
                lock.unlock();
                writer.write(frame);
                logger.info("adding frame");
            }
            logger.info("breaking clip thread loop, renaming");
            writer.release();
            std::filesystem::rename(tmpPath, finalPath); });
        t.detach();
    }

    void resetCounters()
    {
        startCounter = 0;
        staleCounter = 0;
    }
    void incrementStartCount()
    {
        ++startCounter;
    }
    void incrementStaleCount()
    {
        ++staleCounter;
    }

    bool containsCriticalObject(const std::vector<InferenceObjects::DetectedObject> &confidentObjects)
    {

        for (const auto &obj : confidentObjects)
        {
            for (const auto &crit : criticalObject)
            {
                if (crit == obj.type)
                {
                    return true;
                }
            }
        }

        return false;
    }
};