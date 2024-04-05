cmake_minimum_required(VERSION 3.23)
project(FacebowFileReader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

add_executable(FacebowFileReader
    main.cpp
    include/mimetrik/FacebowFileReader.hpp
)

target_link_libraries(FacebowFileReader PRIVATE nlohmann_json::nlohmann_json opencv_core opencv_imgcodecs)

# Python bindings:
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(python-bindings python-bindings.cpp pybind11_opencv.hpp)
target_link_libraries(python-bindings PRIVATE opencv_core)  # we should create a FacebowFileReader library target and then link to that.
set_target_properties(python-bindings PROPERTIES OUTPUT_NAME FacebowFileReader)
