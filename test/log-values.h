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

#ifndef LOG_VALUES_H_
#define LOG_VALUES_H_

#include <list>
#include <mutex>

#include "thread-capture.h"
#include "thread-crosser.h"

namespace capture_thread {

// Captures numerical log entries.
class LogValues : public ThreadCapture<LogValues> {
 public:
  static void Count(int count);

 protected:
  LogValues() = default;
  virtual ~LogValues() = default;

  virtual void LogCount(int count) = 0;
};

// Captures numerical log entries, without automatic thread crossing.
class LogValuesSingleThread : public LogValues {
 public:
  LogValuesSingleThread() : capture_to_(this) {}

  const std::list<int>& GetCounts() { return counts_; }

 private:
  void LogCount(int count) { counts_.emplace_back(count); }

  std::list<int> counts_;
  const ScopedCapture capture_to_;
};

// Captures numerical log entries, with automatic thread crossing.
class LogValuesMultiThread : public LogValues {
 public:
  LogValuesMultiThread() : cross_and_capture_to_(this) {}

  std::list<int> GetCounts();

 private:
  void LogCount(int count);

  std::mutex data_lock_;
  std::list<int> counts_;
  const AutoThreadCrosser cross_and_capture_to_;
};

}  // namespace capture_thread

#endif  // LOG_VALUES_H_
