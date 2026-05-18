if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

cmake_minimum_required(VERSION 3.18)
project(main)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_compile_options(-Wall -Wno-unused-variable)

find_package(OpenCV REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(OpenVINO REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

add_library(serialib STATIC libs/serialib/lib/serialib.cpp)
target_include_directories(serialib PUBLIC libs/serialib/lib)

add_executable(main main.cpp)

target_include_directories(main PRIVATE
    /usr/local/include
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(main
    serialib
    httplib::httplib
    ${OpenCV_LIBS}
    stdc++fs
    openvino::runtime
    onnxruntime
    ${GST_LIBRARIES}
)