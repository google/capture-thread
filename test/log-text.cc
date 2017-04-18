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

#include "log-text.h"

namespace capture_thread {

// static
void LogText::Log(std::string line) {
  if (GetCurrent()) {
    GetCurrent()->LogLine(std::move(line));
  }
}

std::list<std::string> LogTextMultiThread::GetLines() {
  std::lock_guard<std::mutex> lock(data_lock_);
  return lines_;
}

void LogTextMultiThread::LogLine(std::string line) {
  std::lock_guard<std::mutex> lock(data_lock_);
  lines_.emplace_back(std::move(line));
}

}  // namespace capture_thread
