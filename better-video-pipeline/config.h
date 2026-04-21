#pragma once
#include <iostream>
#include <array>
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
        MJPEG,
    };

    constexpr int httpServerPort{3333};

    constexpr MODE displayMode{MODE::RTP_VP8};

    constexpr int bitrate{2000000};
    constexpr std::string_view rtpHost{"127.0.0.1"};
    constexpr int rtpPort{5111};
    constexpr int mjpegPort{5222};
    constexpr int mjpegQuality{80};

    constexpr int frameHeight{1080};
    constexpr int frameWidth{1920};

    constexpr int streamWidth{1280};
    constexpr int streamHeight{720};

    enum ModelFormat
    {
        NONE,
        VINO,
        ONNX
    };

    constexpr float CONF_THRESH{0.5f};

    constexpr std::string_view MODEL_DIR{"models"};
    constexpr ModelFormat MODEL_FORMAT{ModelFormat::VINO};
    constexpr std::string_view MODEL_NAME{"yolo26n"};

    constexpr size_t RESULT_BUFFER_SIZE{3};
    constexpr size_t CAMERA_FRAME_BUFFER_SIZE{3};
    constexpr LogLevel LOG_LEVEL{DEBUG};

    constexpr char *COMPORT{"COM3"};
    constexpr int BAUDRATE{115200};
#ifdef _WIN32
    constexpr auto CAMERA_BACKEND = cv::CAP_MSMF;
    constexpr int CAMERA_INPUT{0};
    constexpr int CAMERA_INFERENCE_INDEX{0};
    constexpr std::array<int, 1> CAMERA_INPUTS{0};
#else
    constexpr std::string_view CAMERA_INPUT{"/dev/video2"};
    constexpr auto CAMERA_BACKEND = cv::CAP_V4L2;
    constexpr int CAMERA_INFERENCE_INDEX{0};
    constexpr std::array<std::string_view, 1> CAMERA_INPUTS{"/dev/video2"};
#endif
}
