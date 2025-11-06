cmake_minimum_required(VERSION 3.10)
project(Gateway CXX C)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
pkg_check_modules(CJSON REQUIRED libcjson)

#C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#resource
add_subdirectory(src)
