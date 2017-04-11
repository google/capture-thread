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

#include <iostream>
#include <list>
#include <string>

#include "thread-capture.h"

using capture_thread::ThreadCapture;

// Inherit from ThreadCapture, passing the class name as the template argument.
class LogText : public ThreadCapture<LogText> {
 public:
  LogText() : capture_to_(this) {}

  // The public *capturing* API is static, and accesses the private API via
  // GetCurrent().
  static void Log(std::string line) {
    if (GetCurrent()) {
      GetCurrent()->LogLine(std::move(line));
    } else {
      std::cerr << "*** Not captured: \"" << line << "\" ***" << std::endl;
    }
  }

  // The public *accessing* API is non-static, and provides the accumulated
  // information in whatever format happens to be useful.
  const std::list<std::string>& GetLines() const { return lines_; }

 private:
  // Never move or copy.
  LogText(const LogText&) = delete;
  LogText(LogText&&) = delete;
  LogText& operator=(const LogText&) = delete;
  LogText& operator=(LogText&&) = delete;
  // Never dynamically allocate.
  void* operator new(std::size_t size) = delete;

  // The private API records data when this logger is being captured to.
  void LogLine(std::string line) { lines_.emplace_back(std::move(line)); }

  // Accumulated data.
  std::list<std::string> lines_;

  // ScopedCapture ensures that the static API logs to this object when:
  // 1. This object is in scope.
  // 2. No logger of the *same* type is higher in the current stack.
  const ScopedCapture capture_to_;
};

void NoLogger() { LogText::Log("No logger is in scope."); }

void LoggedOp1() { LogText::Log("The logger is in scope."); }

void LoggedOp2() { LogText::Log("It captures all lines."); }

int main() {
  NoLogger();
  {
    LogText logger;
    LoggedOp1();
    LoggedOp2();

    for (const auto& line : logger.GetLines()) {
      std::cerr << "Captured: " << line << std::endl;
    }
  }
}
