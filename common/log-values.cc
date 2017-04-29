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

#include "log-values.h"

namespace capture_thread {
namespace testing {

// static
void LogValues::Count(int count) {
  if (GetCurrent()) {
    GetCurrent()->LogCount(count);
  }
}

std::list<int> LogValuesMultiThread::GetCounts() {
  std::lock_guard<std::mutex> lock(data_lock_);
  return counts_;
}

void LogValuesMultiThread::LogCount(int count) {
  std::lock_guard<std::mutex> lock(data_lock_);
  counts_.emplace_back(count);
}

}  // namespace testing
}  // namespace capture_thread
