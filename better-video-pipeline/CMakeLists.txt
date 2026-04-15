if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

cmake_minimum_required(VERSION 3.18)
project(main)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall)

find_package(OpenCV REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

add_executable(main main.cpp libs/serialib/lib/serialib.cpp)

target_include_directories(main PRIVATE
    libs/serialib/lib
    /usr/local/include
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(main
    ${OpenCV_LIBS}
    stdc++fs
    openvino
    onnxruntime
    ${GST_LIBRARIES}
)