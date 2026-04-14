#pragma once
#include <iostream>
#include "logger.h"
#include <opencv2/opencv.hpp>

namespace config
{

    enum MODE
    {
        DISPLAY,
        RTP_VP8,
        RTP_VP9,
        RTP_H264,
        HLS,
    };

    const MODE displayMode{MODE::RTP_VP8};

    const int bitrate{2000000};
    const std::string rtpHost{"127.0.0.1"};
    const int rtpPort{5111};

    const int frameHeight{1080};
    const int frameWidth{1920};

    enum ModelFormat
    {
        NONE,
        VINO,
        ONNX
    };

    constexpr float CONF_THRESH{0.5f};

    const std::string MODEL_DIR{"models"};
    const ModelFormat MODEL_FORMAT{ModelFormat::VINO};
    const std::string MODEL_NAME{"yolo26n"};

    constexpr size_t RESULT_BUFFER_SIZE{3};
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