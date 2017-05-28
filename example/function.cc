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

// This is an example of wrapping a general function that takes arguments and
// has a non-void return.

#include <algorithm>
#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "thread-capture.h"
#include "thread-crosser.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

// (See threaded.cc for comments.)
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

  std::mutex data_lock_;
  std::list<std::string> lines_;
  const AutoThreadCrosser cross_and_capture_to_;
};

// A function whose calls we want to keep track of.
bool LessThan(const std::string& left, const std::string& right) {
  if (left < right) {
    LogText::Log("\"" + left + "\" < \"" + right + "\"");
    return true;
  } else {
    LogText::Log("\"" + left + "\" >= \"" + right + "\"");
    return false;
  }
}

// This simulates a sorting function that might use multiple threads, as a
// hidden implementation detail.
template<class Iterator, class Compare>
void ThreadedSort(Iterator begin, Iterator end, const Compare& compare) {
  std::thread worker([&begin, &end, &compare] {
    std::sort(begin, end, compare);
  });
  worker.join();
}

int main() {
  const std::vector<std::string> words{
        "this", "is", "a", "list", "of", "words", "to", "sort",
      };

  std::vector<std::string> words_copy;

  // Captures log entries while in scope.
  LogText logger;

  words_copy = words;
  // Here we don't know for sure if LessThan is going to be called in this
  // thread or not.
  ThreadedSort(words_copy.begin(), words_copy.end(), &LessThan);

  words_copy = words;
  // ThreadCrosser::WrapFunction ensures that the scope is captured, regardless
  // of how ThreadedSort splits up the process,
  ThreadedSort(words_copy.begin(), words_copy.end(),
               ThreadCrosser::WrapFunction(&LessThan));

  for (const auto& line : logger.CopyLines()) {
    std::cerr << "Captured: " << line << std::endl;
  }
}
