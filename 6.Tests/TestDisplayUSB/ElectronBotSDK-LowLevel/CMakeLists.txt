cmake_minimum_required(VERSION 3.21)
project(ElectronBotSDK-LowLevel)

set(CMAKE_CXX_STANDARD 14)

# 可以将下面的路径单独添加到环境变量OpenCV_DIR
set(OpenCV_DIR "D:\\tool\\opencv2\\opencv\\own-build\\install")

set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs)

find_package(OpenCV REQUIRED)

include_directories(
        src
        lib
        ${OpenCV_INCLUDE_DIRS}
)

file(GLOB_RECURSE SOURCES src/electron_low_level.cpp)

link_directories(
        lib
        ${OpenCV_LIBS}
)

add_library(${PROJECT_NAME} SHARED "src/electron_low_level.cpp")

add_executable(Sample "src/sample.cpp")

target_link_libraries(
        ${PROJECT_NAME}
        USBInterface
        ${OpenCV_LIBS}
        )

target_link_libraries(
        Sample
        ${PROJECT_NAME}
        USBInterface
        ${OpenCV_LIBS}
        )