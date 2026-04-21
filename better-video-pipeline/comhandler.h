#pragma once

#include <opencv2/opencv.hpp>

#include "config.h"

struct ComHandler
{
    std::string port{config::COMPORT};

    ComHandler() = default;

    void sendInferenceResult(cv::Mat msg)
    {

        // convert to some binary format and send
    }
}