#pragma once
#include <iostream>
#include <array>
#include "logger.h"
#include <opencv2/opencv.hpp>

namespace config
{

    bool clippingEnabled{true};
#ifdef _WIN32
    std::string clipDirName{"clips"};
#else
    std::string clipDirName{"/var/lib/streamer/clips"};
#endif
    bool comEnabled{false};

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
    constexpr std::string httpPrefix{"/cpp"};

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

    static const std::array<std::string, 3> MODEL_LABELS{
        "None",
        "OpenVino",
        "ONNX",
    };

    constexpr float CONF_THRESH{0.5f};

#ifdef _WIN32
    constexpr std::string_view MODEL_DIR{"models"};
#else
    constexpr std::string_view MODEL_DIR{"/var/lib/streamer/models"};
#endif
    constexpr ModelFormat MODEL_FORMAT{ModelFormat::VINO};
    constexpr std::string_view MODEL_NAME{"yolo26n"};

    constexpr size_t RESULT_BUFFER_SIZE{3};
    constexpr size_t CAMERA_FRAME_BUFFER_SIZE{3};
    constexpr LogLevel LOG_LEVEL{DEBUG};

    constexpr const char *COMPORT{"COM3"};
    constexpr int BAUDRATE{115200};

    // tracking
    const int DEFAULT_MAX_SPEED{200};
    const int HOME_X{0};
    const int HOME_Y{0};

#ifdef _WIN32
    constexpr auto CAMERA_BACKEND = cv::CAP_MSMF;
    constexpr int CAMERA_INPUT{0};
    constexpr int CAMERA_INFERENCE_INDEX{0};
    constexpr std::array<int, 1> CAMERA_INPUTS{0};
#else
    constexpr std::string_view CAMERA_INPUT{"/dev/video0"};
    constexpr auto CAMERA_BACKEND = cv::CAP_V4L2;
    constexpr int CAMERA_INFERENCE_INDEX{0};
    constexpr std::array<std::string_view, 1> CAMERA_INPUTS{"/dev/video0"};
#endif
}
