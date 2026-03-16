cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall)

find_package(OpenCV REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main ${OpenCV_LIBS})
