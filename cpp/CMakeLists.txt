cmake_minimum_required(VERSION 3.6)
project(cpp_utils)

set(CMAKE_CXX_STANDARD 14)

set(DIR ${CMAKE_CURRENT_LIST_DIR})
set(INCLUDE_DIR ${DIR}/include)
set(SRC_DIR ${DIR}/src)

# Append source files below

set(HEADER_SOURCES
  ${INCLUDE_DIR}/zip.h
  ${INCLUDE_DIR}/active_object.h)

# Append source files above

include_directories(${INCLUDE_DIR})

# TESTING

set(TESTS_DIR ${DIR}/tests)
set(GOOGLE_TEST_DIR ${DIR}/libs/googletest)

include_directories(${GOOGLE_TEST_DIR}/googletest/include)
include_directories(${GOOGLE_TEST_DIR}/googlemock/include)

add_subdirectory(${GOOGLE_TEST_DIR}/googletest ${PROJECT_BINARY_DIR}/googletest)
enable_testing()

set(TEST_SOURCES
  ${TESTS_DIR}/zip_test.cpp
  ${TESTS_DIR}/active_object_test.cpp
)

set(TEST_EXECUTABLE ${PROJECT_NAME}_tests)

add_executable(${TEST_EXECUTABLE} ${TEST_SOURCES})
target_link_libraries(${TEST_EXECUTABLE} gtest gtest_main)

add_test(${TEST_EXECUTABLE} ${TEST_EXECUTABLE})