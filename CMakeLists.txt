cmake_minimum_required(VERSION 3.10)

project(tinyjson VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tinyjson.cc
)

add_library(tinyjson-static STATIC ${SRC_FILES})
set_property(TARGET tinyjson-static PROPERTY OUTPUT_NAME tinyjson)
add_library(tinyjson-shared SHARED ${INT_FILES} ${SRC_FILES})
set_property(TARGET tinyjson-shared PROPERTY OUTPUT_NAME tinyjson)

add_subdirectory(test)
