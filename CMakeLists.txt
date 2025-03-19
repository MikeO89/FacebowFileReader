cmake_minimum_required(VERSION 3.23)
project(FacebowFileReader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VS_JUST_MY_CODE_DEBUGGING ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

add_library(FacebowFileReader INTERFACE)
target_link_libraries(FacebowFileReader INTERFACE nlohmann_json::nlohmann_json opencv_core)
target_sources(FacebowFileReader
	PUBLIC
		FILE_SET api
		TYPE HEADERS
		BASE_DIRS include
		FILES
			include/mimetrik/FacebowFileReader.hpp)

#add_executable(convert-mfba-to-mp4 main.cpp)
add_executable(FacebowFileReaderTest "test/FacebowFileReaderTest.cpp")

#target_link_libraries(convert-mfba-to-mp4 PRIVATE FacebowFileReader opencv_imgcodecs)
target_link_libraries(FacebowFileReaderTest PRIVATE FacebowFileReader GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)


enable_testing()
find_package(GTest CONFIG REQUIRED)
add_test(FacebowFileReaderTest FacebowFileReaderTest)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/resources/test_video_reduced.mfba
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/resources/test_video_empty.mfba
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/resources/test_video_invalid_signature.mfba
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/resources/test_video_invalid_version.mfba
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/resources/test_video_valid.mfba
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/resources/frame0
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test/resources/frame0Meta.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Python bindings:
#find_package(Python COMPONENTS Interpreter Development REQUIRED)
#find_package(pybind11 CONFIG REQUIRED)

#pybind11_add_module(python-bindings python-bindings.cpp pybind11_opencv.hpp)
#target_link_libraries(python-bindings PRIVATE FacebowFileReader)
#set_target_properties(python-bindings PROPERTIES OUTPUT_NAME FacebowFileReader)
#install(TARGETS FacebowFileReader FacebowFileReaderTest convert-mfba-to-mp4 FILE_SET api)
install(TARGETS FacebowFileReader FacebowFileReaderTest FILE_SET api)