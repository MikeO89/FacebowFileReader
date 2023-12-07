cmake_minimum_required(VERSION 3.10)
project(FacebowFileReader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

add_executable(FacebowFileReader
    main.cpp
    FacebowFileReader.hpp
)

target_link_libraries(FacebowFileReader PRIVATE nlohmann_json::nlohmann_json opencv_core opencv_imgcodecs)
