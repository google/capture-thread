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

// ***Although this example is technically safe, its use is a potential
// indicator of bad design, due to scope ambiguity.*** As such, please treat
// this example as experimental, and seriously reconsider your design before
// using this design pattern.

// This is a proof-of-concept example of how you might inherit the object that
// is currently being captured to. For example your API might have multiple
// entry points that delegate to each other, but that requires exactly one
// report per call made to the API.

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

  LogText(InheritType type) : type_(type), capture_to_(this) {}

  static void Log(std::string line) {
    if (GetCurrent()) {
      GetCurrent()->LogLine(std::move(line));
    }
  }

  const std::list<std::string>& GetLines() const { return Delegate()->lines_; }

 private:
  void LogLine(std::string line) {
    Delegate()->lines_.emplace_back(std::move(line));
  }

  LogText* Delegate() {
    if (type_ == InheritType::kInherit && capture_to_.Previous()) {
      return capture_to_.Previous()->Delegate();
    }
    return this;
  }

  const LogText* Delegate() const {
    if (type_ == InheritType::kInherit && capture_to_.Previous()) {
      return capture_to_.Previous()->Delegate();
    }
    return this;
  }

  std::list<std::string> lines_;
  const InheritType type_;
  const ScopedCapture capture_to_;
};

void QueryHandler1(const std::string& query) {
  LogText logger(LogText::InheritType::kInherit);
  LogText::Log("QueryHandler1 called: " + query);
  for (const auto& line : logger.GetLines()) {
    std::cerr << "Available from QueryHandler1: \"" << line << "\""
              << std::endl;
  }
}

void QueryHandler2(const std::string& query) {
  LogText logger(LogText::InheritType::kNew);
  LogText::Log("QueryHandler2 called: " + query);
  QueryHandler1(query + "!!!");
  for (const auto& line : logger.GetLines()) {
    std::cerr << "Available from QueryHandler2: \"" << line << "\""
              << std::endl;
  }
}

int main() {
  std::cerr << "Inherited logger used:" << std::endl;
  QueryHandler2("query");
  std::cerr << "New logger used:" << std::endl;
  QueryHandler1("another query");
}
