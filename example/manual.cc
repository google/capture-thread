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

// This is an example of how you can limit the sharing of instrumentation across
// threads to specific points where you explicitly allow it. This is more
// efficient and makes the code easier to follow than automatic sharing, but it
// doesn't scale well if you have multiple types of instrumentation. (See
// threaded.cc for information about *automatic* sharing of instrumentation.)

#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "thread-capture.h"
#include "thread-crosser.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

// (See simple.cc and threaded.cc for general comments.)
class LogText : public ThreadCapture<LogText> {
 public:
  LogText() : capture_to_(this) {}

  // By default, manually crossing threads is disallowed. To enable it for a
  // specific class, add the `using` declarations below to make these two
  // classes public.
  using ThreadCapture<LogText>::ThreadBridge;
  using ThreadCapture<LogText>::CrossThreads;

  static void Log(std::string line) {
    if (GetCurrent()) {
      GetCurrent()->LogLine(std::move(line));
    } else {
      std::cerr << "*** Not captured: \"" << line << "\" ***" << std::endl;
    }
  }

  std::list<std::string> CopyLines() {
    std::lock_guard<std::mutex> lock(data_lock_);
    return lines_;
  }

 private:
  void LogLine(std::string line) {
    std::lock_guard<std::mutex> lock(data_lock_);
    lines_.emplace_back(std::move(line));
  }

  std::mutex data_lock_;
  std::list<std::string> lines_;

  // Unlike AutoThreadCrosser, ScopedCapture prevents ThreadCrosser from
  // automatically sharing instrumentation across threads.
  const ScopedCapture capture_to_;
};

int main() {
  LogText logger;

  // Instrumentation isn't shared by default.
  std::thread unlogged_thread1(
      [] { LogText::Log("Logging has not been passed on here."); });
  unlogged_thread1.join();

  // Since ScopedCapture is used instead of AutoThreadCrosser, ThreadCrosser::
  // WrapCall has no effect on the LogText instrumentation.
  std::thread unlogged_thread2(ThreadCrosser::WrapCall([] {
    LogText::Log("Logging has not been passed on, even with WrapCall.");
  }));
  unlogged_thread2.join();

  // Use ThreadBridge to create a bridge point. This captures the current scope
  // at the point it's instantiated; therefore, order matters!
  const LogText::ThreadBridge bridge;
  std::thread logged_thread([&bridge] {
    // Connect the threads via the bridge with CrossThreads. The ThreadBridge
    // must stay in scope in the other thread until the CrossThreads in this
    // thread goes out of scope.
    LogText::CrossThreads capture(bridge);
    LogText::Log("Logging has been manually passed on here.");
  });
  logged_thread.join();

  for (const auto& line : logger.CopyLines()) {
    std::cerr << "Captured: " << line << std::endl;
  }
}
