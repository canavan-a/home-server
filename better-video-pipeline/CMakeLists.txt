if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ADAPTER REQUIRED adapter)

add_executable(main main.cpp libs/serialib/lib/serialib.cpp)

target_include_directories(main PRIVATE libs/serialib/lib ${ADAPTER_INCLUDE_DIRS})

link_directories(/usr/local/lib)

target_link_libraries(main 
    ${OpenCV_LIBS} 
    edgetpu 
    /usr/local/lib/libglog.a
    /usr/local/lib/libtensorflow-lite.a
    unwind
    ${ADAPTER_LIBRARIES}
    absl_strings
    absl_strings_internal
    absl_str_format_internal
    absl_base
    absl_raw_logging_internal
    absl_int128
    absl_spinlock_wait
    absl_throw_delegate
)