#include <iostream>
#include <memory>
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <chrono>
#include <semaphore>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <ctime>
#include <sstream>
#include <numeric>
#include <iomanip>
#include <cstdio>
#include <functional>

// git submodules imports
#include <serialib.h>
#include <httplib.h>

// onnx and vino imports
#include <onnxruntime_cxx_api.h>
#include <openvino/openvino.hpp>

#include "constants.h"
#include "ringbuffer.h"
#include "config.h"
#include "logger.h"

#define nl "\n"

struct Frame
{

    Frame() {}
};

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

template <typename T, size_t N>
struct Streamer
{

    std::shared_ptr<RingBuffer<T, N>> buffer;
    std::thread t;

    bool kill{false};

    void stop()
    {
        kill = true;
    }

    Streamer(std::shared_ptr<RingBuffer<T, N>> buf) : buffer{buf}
    {
    }

    void start()
    {
        t = std::thread([this]()
                        { this->run(); });
    }

    virtual ~Streamer()
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    void wait()
    {
        if (t.joinable())
            t.join();
    }

private:
    virtual void run() = 0;
};

template <LogLevel L = LogLevel::INFO>
struct CameraStreamer : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>
{
    std::shared_ptr<std::binary_semaphore> cameraStreamReady = std::make_shared<std::binary_semaphore>(0);
    Logger<L> logger{};
    cv::VideoCapture cap;

    std::string_view cameraInput{config::CAMERA_INPUT};

    bool framerate{};

    CameraStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer)
        : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{buffer}
    {
        this->setup();
        auto countdown = std::vector{3, 2, 1};
        std::ranges::for_each(countdown, [this](auto &v)
                              { 
                                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                                    logger.info(v); });
    }

    CameraStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer, auto camInput)
        : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{buffer}, cameraInput{camInput}
    {
        this->setup();
        logger.info("Starting CamerStreamer with input: " + std::string(cameraInput));
    }

    CameraStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer, bool showFramerate)
        : CameraStreamer{buffer}
    {
        framerate = showFramerate;
    }

    void setup()
    {
        logger.info("setting up camera");
        cap.open(std::string(cameraInput), config::CAMERA_BACKEND);
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH, config::frameWidth);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, config::frameHeight);
        cap.set(cv::CAP_PROP_FPS, 30);

        double fps = cap.get(cv::CAP_PROP_FPS);
        double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
        double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        double fourcc = cap.get(cv::CAP_PROP_FOURCC);

        logger.info("FPS: " + std::to_string(fps));
        logger.info("Width: " + std::to_string(width));
        logger.info("Height: " + std::to_string(height));
    }

    void run() override
    {
        if (!cap.isOpened())
        {
            logger.error("camera is in use by another process");
            return;
        }

        auto count{0};
        cameraStreamReady->release();
        auto emptyFrameCount{0};
        constexpr auto MAX_EMPTY_FRAMES{30};

        static auto last = std::chrono::steady_clock::now();

        while (!kill)
        {

            // logger.debug(++count);
            cv::Mat frame;
            cap >> frame;
            if (frame.empty())
            {
                logger.warn("captured empty frame");
                if (++emptyFrameCount >= MAX_EMPTY_FRAMES)
                {

                    logger.warn("camera disconnected, trying reconnect");
                    cap.release();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    this->setup();
                    emptyFrameCount = 0;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                continue;
            }
            emptyFrameCount = 0;
            logger.info("pushing frame");
            this->buffer->push(frame);
            if (framerate)
            {
                auto now = std::chrono::steady_clock::now();
                float fps = 1.0f / std::chrono::duration<float>(now - last).count();

                std::cout << "CAMERA RAW FPS: " << "\033[32m" << fps << "\033[0m" << "\n";
                last = now;
            }
        }
        cap.release();
    }

    ~CameraStreamer()
    {
        if (t.joinable())
            t.join();

        cap.release();
    }
};

template <LogLevel L = LogLevel::INFO>
struct CaptureManager
{

    std::array<std::string_view, std::size(config::CAMERA_INPUTS)> inputs{config::CAMERA_INPUTS};
    int current{0};
    std::array<std::shared_ptr<CameraStreamer<L>>, config::CAMERA_INPUTS.size()> cameraStreams{};

    CaptureManager()
    {
        this->setup();
    }

    void setup()
    {
        auto index{0};
        std::ranges::for_each(inputs, [&index, this](auto input)
                              {
                                  auto stream = std::make_shared<CameraStreamer<L>>(std::make_shared<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>>(), input);
                                  this->cameraStreams[index++] = std::move(stream); });
    }

    void start()
    {
        std::ranges::for_each(cameraStreams, [this](auto stream)
                              { stream->start(); });
    }

    void stop()
    {
        std::ranges::for_each(cameraStreams, [this](auto stream)
                              { stream->stop(); });
    }

    auto getCurrentBuffer() -> std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>>
    {
        return cameraStreams[current]->buffer;
    }

    auto getSwapBuffer() -> std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>>
    {
        ++current;
        if (current == cameraStreams.size())
        {
            current = 0;
        }
        return getCurrentBuffer();
    }
};

template <LogLevel L = LogLevel::INFO>
struct InferenceConsumer : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>
{

    std::shared_ptr<std::binary_semaphore> cameraStreamReady{};
    Logger<L> logger{};
    cv::dnn::Net net;

    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "yolo"};
    std::unique_ptr<Ort::Session> session;

    ov::Core core;
    ov::CompiledModel model;
    std::unique_ptr<ov::InferRequest> inferenceRequest;

    std::shared_ptr<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>> resultBuffer;

    config::ModelFormat modelFormat{config::MODEL_FORMAT};

    bool inferenceFramerate{};

    // inference result buffer or eat the io overhead..... ??

    InferenceConsumer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer, std::shared_ptr<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>> resBuf, std::shared_ptr<std::binary_semaphore> csReady, config::ModelFormat format)
        : Streamer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>{buffer}, resultBuffer{resBuf}, cameraStreamReady{csReady}, modelFormat{format}
    {
        auto res = LoadModel();
        if (!res)
        {
            logger.error("issue loading in model");
            logger.error(res.error());
            exit(1);
        }
        logger.info("model loaded");
    }

    InferenceConsumer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer, std::shared_ptr<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>> resBuf, std::shared_ptr<std::binary_semaphore> csReady, config::ModelFormat format, bool infRate) : InferenceConsumer{buffer, resBuf, csReady, format}
    {
        inferenceFramerate = infRate;
    }

    void run() override
    {
        logger.info("waiting for camera stream to start");
        cameraStreamReady->acquire();
        logger.info("started inference consumer process");

        static auto last = std::chrono::steady_clock::now();

        while (!kill)
        {

            Result<cv::Mat> peekValue = this->buffer->peekFront();

            if (!peekValue)
            {
                logger.debug("empty frame buffer");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                logger.debug("waited on empty frame");
                continue;
            }

            Result<cv::Mat> result = computeInference(peekValue.value());
            if (!result)
            {
                logger.error("error computing inference");
                logger.error(result.error());
                continue;
            }
            logger.info("inference computed successfully");
            // logger.info(result.value().)

            resultBuffer->push(result.value());

            if (inferenceFramerate)
            {
                auto now = std::chrono::steady_clock::now();
                float fps = 1.0f / std::chrono::duration<float>(now - last).count();

                std::cout << "INFERENCE RAW FPS: " << "\033[96m" << fps << "\033[0m" << "\n";
                last = now;
            }
        }
    }

    Result<cv::Mat> computeInference(const cv::Mat &frame)
    {
        try
        {
            switch (modelFormat)
            {
            case config::ModelFormat::ONNX:
            {
                cv::Mat blob = cv::dnn::blobFromImage(frame, 1 / 255.0, cv::Size(640, 640), cv::Scalar(), true);
                std::array<int64_t, 4> inputShape{1, 3, 640, 640};
                Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
                Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                    memInfo, (float *)blob.data, blob.total(), inputShape.data(), inputShape.size());
                const char *inputNames[] = {"images"};
                const char *outputNames[] = {"output0"};

                auto results = session->Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);
                float *output = results[0].GetTensorMutableData<float>();
                auto shape = results[0].GetTensorTypeAndShapeInfo().GetShape();

                cv::Mat outputMat(shape[1], shape[2], CV_32F, output);
                cv::Mat transposed;
                cv::transpose(outputMat, transposed);

                return transposed;
                break;
            }
            case config::ModelFormat::VINO:
            {
                cv::Mat blob = cv::dnn::blobFromImage(frame, 1 / 255.0, cv::Size(640, 640), cv::Scalar(), true);
                ov::Tensor input = inferenceRequest->get_input_tensor();
                std::memcpy(input.data<float>(), (float *)blob.data, blob.total() * sizeof(float));

                inferenceRequest->infer();

                ov::Tensor output = inferenceRequest->get_output_tensor();
                auto shape = output.get_shape();
                cv::Mat outputMat;
                cv::transpose(cv::Mat(shape[1], shape[2], CV_32F, output.data<float>()), outputMat);
                return outputMat;
                break;
            }
            case config::ModelFormat::NONE:
            {
                logger.info("no inference result, model: NONE");
                return cv::Mat{};
                break;
            }
            default:
                return Err{"invalid model inference type"};
            }

            return Err{"invalid return, unexpected break in InferenceConsumer::cumputeInference"};
        }
        catch (const std::exception &e)
        {
            return Err{e.what()};
        }
    }

    Result<void> LoadModel()
    {
        switch (modelFormat)
        {
        case config::ModelFormat::ONNX:
        {
            std::string onnxPath{std::string(config::MODEL_DIR) + "/" + std::string(config::MODEL_NAME) + ".onnx"};
            if (!std::filesystem::exists(onnxPath))
            {
                return Err{"onnx file not found"};
            }

            Ort::SessionOptions sessionOptions;
            session = std::make_unique<Ort::Session>(env, onnxPath.c_str(), sessionOptions);

            break;
        }
        case config::ModelFormat::VINO:
        {
            std::string vinoBin{std::string(config::MODEL_DIR) + "/" + std::string(config::MODEL_NAME) + ".bin"};
            std::string vinoXml{std::string(config::MODEL_DIR) + "/" + std::string(config::MODEL_NAME) + ".xml"};

            if (!std::filesystem::exists(vinoBin))
            {
                return Err{"openvino bin file not found"};
            }
            if (!std::filesystem::exists(vinoXml))
            {
                return Err{"openvino xml file not found"};
            }

            model = core.compile_model(core.read_model(vinoXml), "CPU");
            inferenceRequest = std::make_unique<ov::InferRequest>(model.create_infer_request());

            break;
        }
        case config::ModelFormat::NONE:
        {
            logger.info("no model format configured");
            break;
        }
        default:
            return Err{"invalid model type"};
        }

        return {};
    }

    ~InferenceConsumer()
    {
        if (t.joinable())
            t.join();
    }
};

template <LogLevel L = LogLevel::INFO>
struct ResultStreamer : Streamer<cv::Mat, config::RESULT_BUFFER_SIZE>
{

    Logger<L> logger{};
    std::mutex signalMutex{};
    std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> cameraBuffer{};

    std::mutex cameraBufferSwapMutex{};

    GstElement *gstPipeline{nullptr};
    GstElement *gstAppsrc{nullptr};
    FILE *ffmpegPipe{nullptr};

    enum COCO
    {
        PERSON = 0,
        CAR = 2,
    };

    const std::array<int, 2> criticalDetections{COCO::CAR, COCO::PERSON};

    const float confidenceThreshold{config::CONF_THRESH};

    config::MODE displayMode{config::displayMode};
    const int port{config::rtpPort};
    const std::string host{config::rtpHost};
    const int bitrate{config::bitrate};

    bool bypassDrawResults{false};
    config::ModelFormat modelFormat{config::MODEL_FORMAT};

    ResultStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>> resBuf, std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> camBuf) : Streamer<cv::Mat, config::RESULT_BUFFER_SIZE>{resBuf}, cameraBuffer{camBuf}
    {
        this->configure();
    }

    void swapCameraStream(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> newCameraBuffer)
    {
        std::lock_guard<std::mutex> lock(this->cameraBufferSwapMutex);
        cameraBuffer = newCameraBuffer;
    }

    void configure()
    {
        if (this->isRtpEnabled())
        {
            configureRtp();
        }
        else if (this->isHlsEnabled())
        {
            configureHls();
        }
        else if (this->isMjpegEnabled())
        {
            configureMjpeg();
        }
    }

    void setDisplayMode(config::MODE newMode)
    {

        displayMode = newMode;

        configure();
    };

    void setModeFormat(const config::ModelFormat format)
    {
        std::lock_guard<std::mutex> lock(this->cameraBufferSwapMutex);

        modelFormat = format;
    }

    void setDrawResultsBypass(bool newValue)
    {
        std::lock_guard<std::mutex> lock(this->cameraBufferSwapMutex);
        bypassDrawResults = newValue;
    }

    bool isRtpEnabled()
    {
        const std::array<config::MODE, 3> rtpFormats{
            config::MODE::RTP_VP8,
            config::MODE::RTP_VP9,
            config::MODE::RTP_H264,
        };

        return std::ranges::find(rtpFormats, displayMode) != rtpFormats.end();
    }

    bool isHlsEnabled()
    {
        const std::array<config::MODE, 1> hlsFormats{
            config::MODE::HLS,
        };

        return std::ranges::find(hlsFormats, displayMode) != hlsFormats.end();
    }

    bool isMjpegEnabled()
    {
        return displayMode == config::MODE::MJPEG;
    }

    void openGstPipeline(const std::string &pipelineStr)
    {
        if (gstPipeline)
        {
            gst_element_set_state(gstPipeline, GST_STATE_NULL);
            if (gstAppsrc)
            {
                gst_object_unref(gstAppsrc);
                gstAppsrc = nullptr;
            }
            gst_object_unref(gstPipeline);
            gstPipeline = nullptr;
        }

        logger.info("opening GStreamer pipeline: " + pipelineStr);
        gstPipeline = gst_parse_launch(pipelineStr.c_str(), nullptr);
        if (!gstPipeline)
        {
            logger.error("GStreamer pipeline failed to create");
            return;
        }

        gstAppsrc = gst_bin_get_by_name(GST_BIN(gstPipeline), "src");

        gst_element_set_state(gstPipeline, GST_STATE_PLAYING);
        logger.info("GStreamer pipeline started successfully");
    }

    std::string appsrcCaps()
    {
        return "video/x-raw,format=BGR,width=" + std::to_string(config::frameWidth) +
               ",height=" + std::to_string(config::frameHeight) + ",framerate=30/1";
    }

    void configureRtp()
    {
        std::string pipelineStr =
            "appsrc name=src is-live=true do-timestamp=true format=time caps=\"" + appsrcCaps() + "\" "
                                                                                                  "! queue max-size-buffers=2 leaky=upstream "
                                                                                                  "! videoconvert "
                                                                                                  "! videoscale ! video/x-raw,width=" +
            std::to_string(config::streamWidth) + ",height=" + std::to_string(config::streamHeight) + " "
                                                                                                      "! vp8enc target-bitrate=" +
            std::to_string(bitrate) + " deadline=1 cpu-used=5 keyframe-max-dist=15 lag-in-frames=0 error-resilient=1 "
                                      "! rtpvp8pay "
                                      "! udpsink host=" +
            host + " port=" + std::to_string(port) + " sync=false async=false";
        openGstPipeline(pipelineStr);
    }

    void configureMjpeg()
    {
        std::string pipelineStr =
            "appsrc name=src is-live=true do-timestamp=true format=time caps=\"" + appsrcCaps() + "\" "
                                                                                                  "! videoconvert "
                                                                                                  "! videoscale ! video/x-raw,width=" +
            std::to_string(config::streamWidth) + ",height=" + std::to_string(config::streamHeight) + " "
                                                                                                      "! jpegenc quality=" +
            std::to_string(config::mjpegQuality) + " "
                                                   "! multipartmux boundary=frame "
                                                   "! souphttpsink port=" +
            std::to_string(config::mjpegPort) + " sync=false async=false";
        openGstPipeline(pipelineStr);
    }

    void configureHls()
    {
        logger.error("hls not configured");
    }

    void run() override
    {
        logger.debug("entering ResultStreamer loop");

        RingBuffer<float, 20> frameRateBuffer{};
        static auto last = std::chrono::steady_clock::now();

        while (!kill)
        {
            std::lock_guard<std::mutex> swapLock(this->cameraBufferSwapMutex);

            // wait for frame signal from the shared buffer
            logger.debug("waiting on tick");
            std::unique_lock<std::mutex> lock(signalMutex);
            logger.debug("tick signaled");

            auto tick = std::chrono::steady_clock::now();
            float fps = 1.0f / std::chrono::duration<float>(tick - last).count();

            last = tick;
            frameRateBuffer.push(fps);

            auto averageFrameRate = std::accumulate(frameRateBuffer.data.begin(), frameRateBuffer.data.end(), 0.0f) / frameRateBuffer.data.size();
            // std::cout << "average framerate: " << averageFrameRate << "\n";

            cameraBuffer->signal.wait(lock);
            // logger.error("triggered ResultStreamer on frame");
            auto frame = cameraBuffer->peekFront();
            auto inferenceResult = this->buffer->peekFront();

            if (!inferenceResult || !frame)
                continue;

            cv::Mat display = frame.value().clone();

            if (modelFormat == config::ModelFormat::NONE || this->bypassDrawResults)
            {
                logger.info("skipping inference processing in ResultStreamer");
            }
            else
            {

                cv::Mat output = inferenceResult.value();

                const float xScale = display.cols / 640.0f;
                const float yScale = display.rows / 640.0f;

                int drawnObjects{};

                std::ranges::for_each(std::views::iota(0, output.rows), [&](int i)
                                      {
                                    cv::Mat scores = output.row(i).colRange(4, output.cols);
                                    cv::Point classIdPoint;
                                    double confidence;
                                    cv::minMaxLoc(scores, nullptr, &confidence, nullptr, &classIdPoint);

                                    int classId = classIdPoint.x;
                                    if (confidence >= confidenceThreshold && std::ranges::any_of(criticalDetections, [classId](int v){ return v == classId; }))
                                    {
                                        float cx = output.at<float>(i, 0) * xScale;
                                        float cy = output.at<float>(i, 1) * yScale;
                                        float w  = output.at<float>(i, 2) * xScale;
                                        float h  = output.at<float>(i, 3) * yScale;

                                        cv::Rect box(
                                            static_cast<int>(cx - w / 2),
                                            static_cast<int>(cy - h / 2),
                                            static_cast<int>(w),
                                            static_cast<int>(h)
                                        );

                                        cv::Scalar colorValue;
                                        if (classId == COCO::CAR){
                                            colorValue=cv::Scalar(0,255,0);
                                        }else{
                                            colorValue=cv::Scalar(0,0,255);
                                        }

                                        cv::rectangle(display, box, colorValue, 2);

                                        ++drawnObjects;
                                    } });
            }

            auto ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << averageFrameRate << " FPS";
            cv::putText(display, oss.str(),
                        cv::Point(10, display.rows - 40),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

            oss.str("");
            oss << std::put_time(std::localtime(&ts), "%Y-%m-%d %H:%M:%S");
            cv::putText(display, oss.str(),
                        cv::Point(10, display.rows - 10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
            handleFrameOutput(display);
            handleInferenceResult(inferenceResult.value());
        }
    }

    void handleInferenceResult(cv::Mat inferenceFrame)
    {
        // reduce and convert to low bandwidth format
        // send over serial or do something else??
    }

    void handleFrameOutput(cv::Mat frame)
    {
        if (displayMode == config::MODE::DISPLAY)
        {
            cv::imshow("detections", frame);
            cv::waitKey(1);
        }
        else if (isRtpEnabled() || isMjpegEnabled())
        {
            if (!gstAppsrc)
            {
                logger.error("appsrc is null, cannot push frame");
                return;
            }

            gsize size = frame.total() * frame.elemSize();
            GstBuffer *buffer = gst_buffer_new_wrapped(g_memdup2(frame.data, size), size);

            GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(gstAppsrc), buffer);
            if (ret != GST_FLOW_OK)
            {
                logger.error("gst_app_src_push_buffer failed: " + std::to_string(ret));
                gst_buffer_unref(buffer);
            }
        }

        else
        {
            logger.warn("display mode not configured code: " + std::to_string(displayMode));
        }
    }

    ~ResultStreamer()
    {
        if (t.joinable())
            t.join();

        if (gstPipeline)
        {
            gst_app_src_end_of_stream(GST_APP_SRC(gstAppsrc));
            gst_element_set_state(gstPipeline, GST_STATE_NULL);
            if (gstAppsrc)
                gst_object_unref(gstAppsrc);
            gst_object_unref(gstPipeline);
        }
    }
};

template <LogLevel L = LogLevel::DEBUG>
struct MediaPipeline
{

    std::unique_ptr<CaptureManager<L>> captureManager;
    std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> cameraBuffer;
    std::shared_ptr<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>> resultBuffer;
    std::unique_ptr<CameraStreamer<L>> cameraStreamer;
    std::unique_ptr<InferenceConsumer<L>> inferenceStreamer;
    std::unique_ptr<ResultStreamer<L>> resultStreamer;
    Logger<L> logger{};

    std::thread httpServerThread;

    httplib::Server svr;

    bool testPrint{};

    MediaPipeline() = default;

    void run(config::ModelFormat format = config::ModelFormat::ONNX)
    {

        // cameraBuffer = std::make_shared<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>>();

        // cameraStreamer = std::make_unique<CameraStreamer<L>>(cameraBuffer, testPrint);
        // cameraStreamer->start();

        captureManager = std::make_unique<CaptureManager<L>>();
        captureManager->start();

        auto inferenceCameraStream = captureManager->cameraStreams[config::CAMERA_INFERENCE_INDEX];

        resultBuffer = std::make_shared<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>>();

        inferenceStreamer = std::make_unique<InferenceConsumer<L>>(inferenceCameraStream->buffer, resultBuffer, inferenceCameraStream->cameraStreamReady, format, testPrint);
        inferenceStreamer->start();

        resultStreamer = std::make_unique<ResultStreamer<L>>(resultBuffer, inferenceCameraStream->buffer);
        resultStreamer->start();

        // startHttpServer();
    }

    void runFor(config::ModelFormat format, std::chrono::seconds seconds)
    {
        run(format);
        std::this_thread::sleep_for(seconds);
        this->stopHttpServer();
        resultStreamer->stop();
        inferenceStreamer->stop();
        // cameraStreamer->stop();
        captureManager->stop();
        logger.debug("stopped camera");
    }

    void startHttpServer()
    {

        svr.Get("/swap", [this](const httplib::Request &req, httplib::Response &res)
                { 
                    this->handleSwapCamera();
                    res.status=200;
                    res.set_content("Camera swap triggered", "text/plain"); });

        logger.info("starting http server");
        httpServerThread = std::thread([this]()
                                       { svr.listen("0.0.0.0", config::httpServerPort); });
    }

    void stopHttpServer()
    {

        logger.info("stopping http server");
        svr.stop();

        if (httpServerThread.joinable())
        {
            httpServerThread.join();
        }
    }

    void handleSwapCamera()
    {
        resultStreamer->swapCameraStream(captureManager->getSwapBuffer());
    }

    ~MediaPipeline()
    {
        if (httpServerThread.joinable())
            httpServerThread.join();
    }
};

struct TestProgram
{
    TestProgram() = default;

    void test()
    {
        auto mp = MediaPipeline<LogLevel::ERROR>{};
        mp.testPrint = true;
        mp.runFor(config::ModelFormat::ONNX, std::chrono::seconds(5));

        std::cout << "\n"
                  << " done with onnx" << "\n\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));

        mp.runFor(config::ModelFormat::VINO, std::chrono::seconds(5));
    }
};

int main(int argc, char *argv[])
{

    gst_init(nullptr, nullptr);

    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    std::cout << cv::getBuildInformation() << std::endl;

    serialib serial;
    auto res = serial.openDevice(config::COMPORT, config::BAUDRATE);

    Logger<DEBUG> log{};

    log.info("res is: " + res);

    log.debug("debug");

    log.warn("warn");

    log.info("info");

    log.error("error");

    std::string flag1{};

    // parse args
    if (argc >= 2)
    {
        // args are good
        flag1 = argv[1];
    }

    if (flag1 == "test")
    {
        TestProgram t{};

        t.test();
    }
    else
    {
        auto mp = MediaPipeline<LogLevel::INFO>{};
        mp.run(config::MODEL_FORMAT);
    }

    return 0;
}