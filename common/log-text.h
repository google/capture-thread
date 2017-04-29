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

#ifndef LOG_TEXT_H_
#define LOG_TEXT_H_

#include <list>
#include <mutex>
#include <string>

#include "thread-capture.h"
#include "thread-crosser.h"

namespace capture_thread {
namespace testing {

// Captures text log entries.
class LogText : public ThreadCapture<LogText> {
 public:
  static void Log(std::string line);

 protected:
  LogText() = default;
  virtual ~LogText() = default;

  virtual void LogLine(std::string line) = 0;
};

// Captures text log entries, without automatic thread crossing.
class LogTextSingleThread : public LogText {
 public:
  LogTextSingleThread() : capture_to_(this) {}

  const std::list<std::string>& GetLines() { return lines_; }

 private:
  void LogLine(std::string line) override {
    lines_.emplace_back(std::move(line));
  }

  std::list<std::string> lines_;
  const ScopedCapture capture_to_;
};

// Captures text log entries, with automatic thread crossing.
class LogTextMultiThread : public LogText {
 public:
  LogTextMultiThread() : cross_and_capture_to_(this) {}

  std::list<std::string> GetLines();

 private:
  void LogLine(std::string line) override;

  std::mutex data_lock_;
  std::list<std::string> lines_;
  const AutoThreadCrosser cross_and_capture_to_;
};

}  // namespace testing
}  // namespace capture_thread

#endif  // LOG_TEXT_H_
