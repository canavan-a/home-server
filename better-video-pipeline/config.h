#pragma once
#include <iostream>
#include "logger.h"
#include <opencv2/opencv.hpp>

namespace config
{
    enum ModelFormat
    {
        VINO,
        ONNX
    };

    const std::string MODEL_DIR{"models"};
    const ModelFormat MODEL_FORMAT{ModelFormat::VINO};
    const std::string MODEL_NAME{"yolo26n"};

    constexpr size_t CAMERA_FRAME_BUFFER_SIZE{3};
    constexpr LogLevel LOG_LEVEL{DEBUG};

    const char *COMPORT{"COM3"};
    const int BAUDRATE{115200};
    const int RTP_PORT{3000};
#ifdef _WIN32
    constexpr auto CAMERA_BACKEND = cv::CAP_MSMF;
    const int CAMERA_INPUT{0};
#else
    const std::string CAMERA_INPUT{"/dev/video0"};
    constexpr auto CAMERA_BACKEND = cv::CAP_V4L2;
#endif
}