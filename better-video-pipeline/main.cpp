#include <iostream>
#include <memory>
#include <thread>
#include <serialib.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <chrono>
#include <semaphore>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <ctime>
#include <sstream>

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

    CameraStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> buffer, bool showFramerate)
        : CameraStreamer{buffer}
    {
        framerate = showFramerate;
    }

    void setup()
    {
        logger.info("setting up camera");
        cap.open(config::CAMERA_INPUT, config::CAMERA_BACKEND);
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
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
        std::cout << "hello world I am a Camera streamer" << nl;
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
                float *data = output.data<float>();
                auto shape = output.get_shape();

                cv::Mat outputMat(shape[1], shape[2], CV_32F, data);
                cv::Mat transposed;
                cv::transpose(outputMat, transposed);
                return transposed;
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
            std::string onnxPath{config::MODEL_DIR + "/" + config::MODEL_NAME + ".onnx"};
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
            std::string vinoBin{config::MODEL_DIR + "/" + config::MODEL_NAME + ".bin"};
            std::string vinoXml{config::MODEL_DIR + "/" + config::MODEL_NAME + ".xml"};

            if (!std::filesystem::exists(vinoBin))
            {
                return Err{"openvino bin file not found"};
            }
            if (!std::filesystem::exists(vinoXml))
            {
                return Err{"openvino xml file not found"};
            }

            model = core.compile_model(vinoXml, "CPU");
            inferenceRequest = std::make_unique<ov::InferRequest>(model.create_infer_request());

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

    enum COCO
    {
        PERSON = 0,
        CAR = 2,
    };

    const std::array<int, 2> criticalDetections{COCO::CAR, COCO::PERSON};

    const float confidenceThreshold{config::CONF_THRESH};

    ResultStreamer(std::shared_ptr<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>> resBuf, std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> camBuf) : Streamer<cv::Mat, config::RESULT_BUFFER_SIZE>{resBuf}, cameraBuffer{camBuf}
    {
    }

    void run() override
    {
        while (!kill)
        {
            // wait for frame signal from the shared buffer
            std::unique_lock<std::mutex> lock(signalMutex);
            cameraBuffer->signal.wait(lock);
            logger.error("triggered ResultStreamer on frame");
            auto frame = cameraBuffer->peekFront();
            auto inferenceResult = this->buffer->peekFront();

            if (!inferenceResult || !frame)
                continue;

            cv::Mat output = inferenceResult.value();
            cv::Mat display = frame.value().clone();

            const float xScale = display.cols / 640.0f;
            const float yScale = display.rows / 640.0f;

            int drawnObjects{};

            std::cout << "[ResultStreamer] output rows=" << output.rows << " cols=" << output.cols << "\n";

            double globalMax = 0;
            std::ranges::for_each(std::views::iota(0, output.rows), [&](int i)
                                  {
                                    cv::Mat scores = output.row(i).colRange(4, output.cols);
                                    cv::Point classIdPoint;
                                    double confidence;
                                    cv::minMaxLoc(scores, nullptr, &confidence, nullptr, &classIdPoint);

                                    if (confidence > globalMax) globalMax = confidence;

                                    int classId = classIdPoint.x;
                                    std::cout << "  row=" << i << " classId=" << classId << " conf=" << confidence << "\n";

                                    if (confidence >= 0.5)
                                    {
                                        std::cout << "[DETECTION] class=" << classId
                                                  << " conf=" << confidence << "\n";

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

            std::cout << "[ResultStreamer] best confidence this frame=" << globalMax << "\n";

            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::ostringstream oss;
            oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
            std::string timestamp = oss.str();
            cv::putText(display, timestamp,
                        cv::Point(10, display.rows - 10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

            cv::imshow("detections", display);
            cv::waitKey(1);

            if (drawnObjects > 0)
            {
                logger.error("objects drawn");
            }
        }
    }

    ~ResultStreamer()
    {
        if (t.joinable())
        {
            t.join();
        }
    }
};

template <LogLevel L = LogLevel::DEBUG>
struct MediaPipeline
{
    std::shared_ptr<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>> cameraBuffer;
    std::shared_ptr<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>> resultBuffer;
    std::unique_ptr<CameraStreamer<L>> cameraStreamer;
    std::unique_ptr<InferenceConsumer<L>> inferenceStreamer;
    std::unique_ptr<ResultStreamer<L>> resultStreamer;
    Logger<L> logger{};

    bool testPrint{};

    MediaPipeline() = default;

    void run(config::ModelFormat format = config::ModelFormat::ONNX)
    {
        cameraBuffer = std::make_shared<RingBuffer<cv::Mat, config::CAMERA_FRAME_BUFFER_SIZE>>();

        cameraStreamer = std::make_unique<CameraStreamer<L>>(cameraBuffer, testPrint);
        cameraStreamer->start();

        resultBuffer = std::make_shared<RingBuffer<cv::Mat, config::RESULT_BUFFER_SIZE>>();

        inferenceStreamer = std::make_unique<InferenceConsumer<L>>(cameraBuffer, resultBuffer, cameraStreamer->cameraStreamReady, format, testPrint);
        inferenceStreamer->start();

        resultStreamer = std::make_unique<ResultStreamer<L>>(resultBuffer, cameraBuffer);
        resultStreamer->start();
    }

    void runFor(config::ModelFormat format, std::chrono::seconds seconds)
    {
        run(format);
        std::this_thread::sleep_for(seconds);
        resultStreamer->stop();
        inferenceStreamer->stop();
        cameraStreamer->stop();
        logger.debug("stopped camera");
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
        auto mp = MediaPipeline<LogLevel::ERROR>{};
        mp.run(config::ModelFormat::VINO);
    }

    return 0;
}