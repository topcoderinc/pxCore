cmake_minimum_required (VERSION 2.6)
project (gstpipeline)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMERBASE REQUIRED gstreamer-app-1.0)

include_directories(${GSTREAMER_INCLUDE_DIRS})

add_library(gstpipeline px_gst_pipeline.cpp)
