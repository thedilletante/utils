cmake_minimum_required(VERSION 3.6)
project(cpp_utils)

set(CMAKE_CXX_STANDARD 14)

set(DIR ${CMAKE_CURRENT_LIST_DIR})
set(INCLUDE_DIR ${DIR}/include)
set(SRC_DIR ${DIR}/src)

set(SOURCES
  ${INCLUDE_DIR}/active_object.h
  ${INCLUDE_DIR}/merge_allocator.h
  ${INCLUDE_DIR}/zip.h
  ${SRC_DIR}/merge_allocator.cpp
)

add_library(${PROJECT_NAME}
  ${SOURCES}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${INCLUDE_DIR}
)

# TESTING
set(TESTS_DIR ${DIR}/tests)
set(GOOGLE_TEST_DIR ${DIR}/libs/googletest)

add_subdirectory(${GOOGLE_TEST_DIR}/googletest ${PROJECT_BINARY_DIR}/googletest)
enable_testing()

set(TEST_SOURCES
  ${TESTS_DIR}/zip_test.cpp
  ${TESTS_DIR}/active_object_test.cpp
  ${TESTS_DIR}/merge_allocator_test.cpp
)

set(TEST_EXECUTABLE ${PROJECT_NAME}_tests)

add_executable(${TEST_EXECUTABLE} ${TEST_SOURCES})
target_include_directories(
  ${TEST_EXECUTABLE}
    PRIVATE
      ${GOOGLE_TEST_DIR}/googletest/include
      ${GOOGLE_TEST_DIR}/googlemock/include
)
target_link_libraries(${TEST_EXECUTABLE} gtest gtest_main ${PROJECT_NAME})
add_test(${TEST_EXECUTABLE} ${TEST_EXECUTABLE})

add_executable(mutex
    apps/mutex.cpp
)
target_link_libraries(mutex pthread)
