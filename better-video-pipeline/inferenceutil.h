#pragma once
#include "config.h"
#include <opencv2/opencv.hpp>
#include "coco.h"

struct Object
{
    COCO type{};
    float x1, x2, y1, y2;
    float confidence{};
};