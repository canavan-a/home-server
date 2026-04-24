#pragma once

#include "config.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <array>
#include <ranges>
#include "coco.h"

namespace InferenceObjects
{
    struct DetectedObject
    {
        COCO type{};
        float x1, x2, y1, y2;
        float confidence{};
    };

    // inferenceResult is the transposed YOLO output: [N, 4+classes], coords in 640x640 space
    auto parseObjects(cv::Mat inferenceResult, float confidenceThreshold) -> std::vector<DetectedObject>
    {
        static constexpr std::array<int, 2> validClasses{COCO::PERSON, COCO::CAR};

        std::vector<DetectedObject> objs{};

        for (int i = 0; i < inferenceResult.rows; ++i)
        {
            cv::Mat scores = inferenceResult.row(i).colRange(4, inferenceResult.cols);
            cv::Point classIdPoint;
            double confidence;
            cv::minMaxLoc(scores, nullptr, &confidence, nullptr, &classIdPoint);

            int classId = classIdPoint.x;
            if (confidence < confidenceThreshold)
                continue;
            if (!std::ranges::any_of(validClasses, [classId](int v) { return v == classId; }))
                continue;

            float cx = inferenceResult.at<float>(i, 0);
            float cy = inferenceResult.at<float>(i, 1);
            float w  = inferenceResult.at<float>(i, 2);
            float h  = inferenceResult.at<float>(i, 3);

            objs.push_back({
                static_cast<COCO>(classId),
                cx - w / 2.0f,
                cx + w / 2.0f,
                cy - h / 2.0f,
                cy + h / 2.0f,
                static_cast<float>(confidence)
            });
        }

        return objs;
    }
}