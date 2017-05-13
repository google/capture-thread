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

// This is a minimal example of creating functionality that can cross threads.
// This is in contrast to simple.cc, which is strictly compartmentalized to
// individual threads.

#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "thread-capture.h"
#include "thread-crosser.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

// (See simple.cc for comments. The only comments here relate to threading.)
class LogText : public ThreadCapture<LogText> {
 public:
  LogText() : cross_and_capture_to_(this) {}

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

  // To support threading, just add mutex protection and an AutoThreadCrosser.
  // AutoThreadCrosser ensures that logging is passed on to worker threads, but
  // *only* when the thread function is wrapped with ThreadCrosser::WrapCall.

  // Note that if you implement multiple types of ThreadCapture in this way, a
  // single call to ThreadCrosser::WrapCall will pass on *all* of them that are
  // currently in scope in the current thread. For example, if you're logging
  // strings (e.g., this class) and have a separate ThreadCapture to log timing
  // that also uses AutoThreadCrosser, a single call to ThreadCrosser::WrapCall
  // will pass on both string logging and timing logging.

  std::mutex data_lock_;
  std::list<std::string> lines_;
  const AutoThreadCrosser cross_and_capture_to_;
};

void NoLogger() { LogText::Log("No logger is in scope."); }

void LoggedOp() { LogText::Log("The logger is in scope."); }

void LoggedOpInThread() {
  LogText::Log("ThreadCrosser::WrapCall passes on logging.");
}

void UnloggedOpInThread() {
  LogText::Log("Logging has not been passed on here.");
}

int main() {
  NoLogger();
  {
    LogText logger;
    LoggedOp();

    std::thread logged_thread(ThreadCrosser::WrapCall(&LoggedOpInThread));
    std::thread unlogged_thread(&UnloggedOpInThread);

    logged_thread.join();
    unlogged_thread.join();

    for (const auto& line : logger.CopyLines()) {
      std::cerr << "Captured: " << line << std::endl;
    }
  }
}
