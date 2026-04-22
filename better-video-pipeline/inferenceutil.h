#pragma once

#include "config.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include "coco.h"

namespace InferenceObjects
{
    struct DetectedObject
    {
        COCO type{};
        float x1, x2, y1, y2;
        float confidence{};
    };

    auto parseObjects(cv::Mat inferenceResult, float confidence) std::vector<DetectedObject>
    {

        std::vector<DetectedObject> objs{};
    }
}