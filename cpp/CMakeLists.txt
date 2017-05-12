cmake_minimum_required(VERSION 3.7)
project(cpp_utils)

set(CMAKE_CXX_STANDARD 14)

set(DIR ${CMAKE_CURRENT_LIST_DIR})
set(GOOGLE_TEST_DIR ${DIR}/libs/googletest)

set(INCLUDE_DIR ${DIR}/include)
set(SRC_DIR ${DIR}/src)
set(TESTS_DIR ${DIR}/tests)

include_directories(${INCLUDE_DIR})

set(SOURCES
  ${SRC_DIR}/zip.cpp
  ${INCLUDE_DIR}/zip.h)

set(TEST_SOURCES
  ${TESTS_DIR}/zip_test.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})


# TESTING

include_directories(${GOOGLE_TEST_DIR}/googletest/include)
include_directories(${GOOGLE_TEST_DIR}/googlemock/include)

add_subdirectory(${GOOGLE_TEST_DIR}/googletest ${PROJECT_BINARY_DIR}/googletest)
enable_testing()

add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}_tests gtest gtest_main ${PROJECT_NAME})

add_test(${PROJECT_NAME}_tests ${PROJECT_NAME}_tests)