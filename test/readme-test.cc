/* -----------------------------------------------------------------------------
Copyright 2017 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
----------------------------------------------------------------------------- */

// Author: Kevin P. Barry [ta0kira@gmail.com] [kevinbarry@google.com]

// Code used in the quick-start in README.md. Make sure the two stay in sync!


// STEP 1

#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include "thread-capture.h"

// This class provides the instrumentation logic, both at the point the
// instrumentation is used (e.g., logging points) and where it is enabled (e.g.,
// log-capture points.) Note that instances of ThreadCapture cannot be moved,
// copied, or dynamically allocated.
class Logger : capture_thread::ThreadCapture<Logger> {
 public:
  Logger() : cross_and_capture_to_(this) {}

  // The static API is used at the instrumentation points. It will often be a
  // no-op if no instrumentation is in scope.
  static void Log(const std::string& line) {
    // GetCurrent() provides the instrumentation currently in scope, and is
    // always thread-safe and repeatable. The implementation of the
    // instrumentation must be explicitly made thread-safe, however.
    if (GetCurrent()) {
      GetCurrent()->LogLine(line);
    } else {
      std::cerr << "Not captured: \"" << line << "\"" << std::endl;
    }
  }

  // The non-static public API allows the creator of the instrumentation object
  // to access its contents. This is only necessary when the instrumentation is
  // gathering information, as opposed to propagating information.
  std::list<std::string> GetLines() {
    std::lock_guard<std::mutex> lock(lock_);
    return lines_;
  }

 private:
  // The private implementation applies to the instrumentation only when it's in
  // scope. This does not need to exactly mirror the static API, and in fact
  // only needs to differentiate between default and override behaviors.
  void LogLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(lock_);
    lines_.emplace_back(line);
  }

  std::mutex lock_;
  std::list<std::string> lines_;
  // Add an AutoThreadCrosser to ensure that scoping is handled correctly. If
  // you absolutely don't want the instrumentation crossing threads, use
  // ScopedCapture instead. Always initialize with `this`.
  const AutoThreadCrosser cross_and_capture_to_;
};


// STEP 2

// #include the header for your instrumentation class.

// This function already exists in your code, and performs some sort of work for
// which you want to use the instrumentation.
void MyExistingFunction() {
  // Add calls to the static API where you need access to the instrumentation.
  Logger::Log("MyExistingFunction called");
}


// STEP 3

#include <thread>
#include "thread-crosser.h"

// (You don't need to #include the header for your instrumentation class here.)

// This function already exists in your code, and parallelizes some
// functionality that needs to use the instrumentation, but doesn't need to use
// the instrumentation itself.
void ParallelizeWork() {
  // Previously, the code just created a thread.
  // std::thread worker(&MyExistingFunction);

  // To pass along the instrumentation, wrap the thread with WrapCall. Also use
  // WrapCall when passing work to a worker thread, e.g., a thread pool.
  std::thread worker(
      capture_thread::ThreadCrosser::WrapCall(&MyExistingFunction));
  worker.join();
}


// STEP 4

// #include the header for your instrumentation class.

int main() {
  // If no instrumentation is in scope, the default behavior of the static API
  // is used where instrumentation calls are made. In this case, this will just
  // print the line to std::cerr.
  ParallelizeWork();

  // To make the instrumentation available within a given scope, just
  // instantiate your class. The framework will take care of the rest.
  Logger logger;

  // Since a Logger is in scope, the line will be captured to that instance,
  // rather than the default behavior of printing to std::cerr.
  ParallelizeWork();

  // In this case the instrumentation captures data, which is now available in
  // the local instance.
  for (const std::string& line : logger.GetLines()) {
    std::cerr << "The logger captured: \"" << line << "\"" << std::endl;
  }
}
