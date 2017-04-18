cmake_minimum_required(VERSION 2.6)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/common)

set(CMAKE_CXX_FLAGS "-Wall -pedantic -std=c++11 -O2 -g -pthread")

add_library(
  capture-thread
  src/thread-crosser.cc)

add_executable(
  simple
  example/simple.cc)
target_link_libraries(
  simple
  capture-thread)

add_executable(
  threaded
  example/threaded.cc)
target_link_libraries(
  threaded
  capture-thread
  pthread)

add_executable(
  limit
  example/limit.cc)
target_link_libraries(
  limit
  capture-thread)

add_executable(
  mock
  example/mock.cc)
target_link_libraries(
  mock
  capture-thread)

add_executable(
  trace
  example/trace.cc)
target_link_libraries(
  trace
  capture-thread
  pthread)

add_executable(
  demo-main
  demo/main.cc
  demo/logging.cc
  demo/tracing.cc
  common/callback-queue.cc)
target_link_libraries(
  demo-main
  capture-thread
  pthread)

find_package(GTest)
if(GTEST_LIBRARIES)

  include_directories(${GTEST_INCLUDE_DIRS})

  add_executable(
    capture-thread-test
    test/test.cc
    test/log-text.cc
    test/log-values.cc
    common/callback-queue.cc)
  target_link_libraries(
    capture-thread-test
    gtest gmock gtest_main
    capture-thread pthread)

  add_executable(
    demo-test
    demo/test.cc
    demo/logging.cc
    demo/tracing.cc
    common/callback-queue.cc)
  target_link_libraries(
    demo-test
    gtest gmock gtest_main
    capture-thread pthread)

endif()
