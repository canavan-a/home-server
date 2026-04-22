#pragma once

#include <opencv2/opencv.hpp>
#include <array>
#include <ranges>
#include <

#include "inferenceutil.h"
#include "config.h"
#include "ringbuffer.h"

constexpr int PRECLIP_QUEUE_SIZE{150};
constexpr int CLIP_START_THRESH{20}; // frames to start the clip
constexpr int CLIP_STALE_THRESH{35}; // frames to stop the clip

struct ClipHandler
{

    RingBuffer<cv::Mat, PRECLIP_QUEUE_SIZE> preclip{};

    const std::array<COCO, 1> criticalObject{COCO::PERSON};

    bool clipping{false};
    int startCounter{0};
    int staleCounter{0};

    std::vector<cv::Mat> currentClip{};

    ClipHandler() = default;

    // objects are deemed to already be filtered for confidence
    void handleInferenceFrame(cv::Mat frame, const std::vector<InferenceObjects::DetectedObject> &confidentObjects)
    {
        // add preclip
        if (!this->containsCriticalObject(confidentObjects))
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
            currentClip.push_back(frame);
            // check length and see if clip is too long. if so .... force end the clip
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
                startClip()
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
            return
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
        resetCounters();
        clipping = true;
    }

    void endClip()
    {
        resetCounters();
        clipping = false;

        // combine and send buffers to a processing thread
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
        --staleCounter;
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