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

// This is a proof-of-concept example of how you might inherit the object that
// is currently being captured to. This use-case requires some more thought, so
// this example will likely become obsolete.

#include <iostream>
#include <list>
#include <string>

#include "thread-capture.h"

using capture_thread::ThreadCapture;

// (See simple.cc for general comments.)
class LogText : public ThreadCapture<LogText> {
 public:
  enum class InheritType {
    kNew,      // A new logger should be created.
    kInherit,  // Logging should be delegated to the existing logger.
  };

  LogText(InheritType type)
      : type_(type),
        capture_to_(type_ == InheritType::kInherit ? GetCurrent() : this) {}

  static void Log(std::string line) {
    if (GetCurrent()) {
      GetCurrent()->LogLine(std::move(line));
    }
  }

  const std::list<std::string>& GetLines() const {
    if (type_ == InheritType::kInherit && capture_to_.Previous()) {
      return capture_to_.Previous()->lines_;
    } else {
      return lines_;
    }
  }

 private:
  void LogLine(std::string line) { lines_.emplace_back(std::move(line)); }

  std::list<std::string> lines_;
  const InheritType type_;
  const ScopedCapture capture_to_;
};

int main() {
  LogText logger1(LogText::InheritType::kNew);
  LogText::Log("Captured to logger1.");
  {
    LogText logger2(LogText::InheritType::kInherit);
    LogText::Log("Also captured to logger1, due to delegation.");
  }
  for (const auto& line : logger1.GetLines()) {
    std::cerr << "Captured: " << line << std::endl;
  }
}
