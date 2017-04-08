cmake_minimum_required(VERSION 2.6)

include_directories(
  ${CMAKE_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "-Wall -pedantic -std=c++11 -O2 -g")

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

find_package(GTest)
if(GTEST_LIBRARIES)

  include_directories(${GTEST_INCLUDE_DIRS})

  add_executable(
    capture-thread-test
    test/test.cc)
  target_link_libraries(
    capture-thread-test
    gtest gmock gtest_main
    capture-thread pthread)

endif()
