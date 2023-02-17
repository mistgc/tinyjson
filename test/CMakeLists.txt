cmake_minimum_required(VERSION 3.10)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(TEST_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cc
)

add_executable(tinyjson_test ${TEST_SRC_FILES})
target_link_libraries(tinyjson_test tinyjson-static)
