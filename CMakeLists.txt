cmake_minimum_required(VERSION 2.6)
project(capture-thread)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/common)

set(CMAKE_CXX_FLAGS "-Wall -pedantic -std=c++11 -O2 -g -pthread")

IF(NOT USE_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")
ENDIF()

SET(INSTALL_INCLUDEDIR "include")
SET(INSTALL_LIBDIR "lib")

SET(INSTALL_PATH "${CMAKE_INSTALL_PREFIX}")
SET(INCLUDE_PATH "${INSTALL_PATH}/${INSTALL_INCLUDEDIR}")
SET(LIBRARY_PATH "${INSTALL_PATH}/${INSTALL_LIBDIR}")

INCLUDE(FindThreads)
FIND_LIBRARY(PTHREAD_LIBRARY
  NAMES pthread
  QUIET
  ONLY_CMAKE_FIND_ROOT_PATH)
IF(NOT PTHREAD_LIBRARY)
 SET(PTHREAD_LIBRARY "")
ENDIF()

add_library(capture-thread STATIC
  src/thread-crosser.cc)

add_executable(async
  example/async.cc)
target_link_libraries(async
  capture-thread
  ${PTHREAD_LIBRARY})

add_executable(inherit
  example/inherit.cc)
target_link_libraries(inherit
  capture-thread)

add_executable(limit
  example/limit.cc)
target_link_libraries(
  limit
  capture-thread)

add_executable(mock
  example/mock.cc)
target_link_libraries(mock
  capture-thread)

add_executable(multi
  example/multi.cc)
target_link_libraries(multi
  capture-thread
  ${PTHREAD_LIBRARY})

add_executable(paths
  example/paths.cc)
target_link_libraries(
  paths
  capture-thread)

add_executable(simple
  example/simple.cc)
target_link_libraries(
  simple
  capture-thread)

add_executable(speed
  example/speed.cc)
target_link_libraries(
  speed
  capture-thread)

add_executable(threaded
  example/threaded.cc)
target_link_libraries(
  threaded
  capture-thread
  ${PTHREAD_LIBRARY})

add_executable(throttle
  example/throttle.cc)
target_link_libraries(
  throttle
  capture-thread
  ${PTHREAD_LIBRARY})

add_executable(trace
  example/trace.cc)
target_link_libraries(
  trace
  capture-thread
  ${PTHREAD_LIBRARY})

add_executable(demo-main
  demo/main.cc
  demo/logging.cc
  demo/tracing.cc
  common/callback-queue.cc)
target_link_libraries(demo-main
  capture-thread
  ${PTHREAD_LIBRARY})

add_executable(readme-test
  test/readme-test.cc)
target_link_libraries(readme-test
  capture-thread
  ${PTHREAD_LIBRARY})

INSTALL(TARGETS
  capture-thread
  DESTINATION ${INSTALL_LIBDIR})

INSTALL(FILES
  include/thread-capture.h
  include/thread-crosser.h
  DESTINATION ${INSTALL_INCLUDEDIR})

find_package(GTest)
if(GTEST_LIBRARIES)

  include_directories(${GTEST_INCLUDE_DIRS})

  add_executable(thread-capture-test
    test/thread-capture-test.cc
    common/log-text.cc
    common/log-values.cc
    common/callback-queue.cc)
  target_link_libraries(thread-capture-test
    gtest gmock gtest_main
    capture-thread
    ${PTHREAD_LIBRARY})

  add_executable(thread-crosser-test
    test/thread-crosser-test.cc
    common/log-text.cc
    common/log-values.cc
    common/callback-queue.cc)
  target_link_libraries(thread-crosser-test
    gtest gmock gtest_main
    capture-thread
    ${PTHREAD_LIBRARY})

  add_executable(demo-test
    demo/test.cc
    demo/logging.cc
    demo/tracing.cc
    common/callback-queue.cc)
  target_link_libraries(demo-test
    gtest gmock gtest_main
    capture-thread
    ${PTHREAD_LIBRARY})

endif()
