cmake_minimum_required(VERSION 3.24)
project(AndiyaNativeInvert LANGUAGES CXX)
set(ANDIYA_SDK_INCLUDE "" CACHE PATH "Directory containing andiya/plugin_api.h")
if(NOT EXISTS "${ANDIYA_SDK_INCLUDE}/andiya/plugin_api.h")
    message(FATAL_ERROR "Set ANDIYA_SDK_INCLUDE to the source include or installed sdk/include directory")
endif()
add_library(AndiyaNativeInvert SHARED plugin.cpp)
target_compile_features(AndiyaNativeInvert PRIVATE cxx_std_17)
target_include_directories(AndiyaNativeInvert PRIVATE "${ANDIYA_SDK_INCLUDE}")
set_target_properties(AndiyaNativeInvert PROPERTIES
    OUTPUT_NAME invert
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/package>"
    LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/package>")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
configure_file(plugin.json.in "${CMAKE_BINARY_DIR}/package/plugin.json" @ONLY)
configure_file(LICENSE "${CMAKE_BINARY_DIR}/package/LICENSE" COPYONLY)
