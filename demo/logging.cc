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

#include "logging.h"
#include "tracing.h"

namespace demo {

Logging::LogLine::LogLine() : capture_(GetCurrent()) {
  const std::string context = Tracing::GetContext();
  if (!context.empty()) {
    *this << context << ": ";
  } else {
    *this << "(unknown context): ";
  }
}

Logging::LogLine::~LogLine() {
  *this << '\n';
  if (capture_) {
    capture_->AppendLine(output_.str());
  } else {
    DefaultAppendLine(output_.str());
  }
}

// static
void Logging::DefaultAppendLine(const std::string& line) { std::cerr << line; }

std::list<std::string> CaptureLogging::CopyLines() {
  std::lock_guard<std::mutex> lock(data_lock_);
  return lines_;
}

void CaptureLogging::AppendLine(const std::string& line) {
  {
    std::lock_guard<std::mutex> lock(data_lock_);
    lines_.emplace_back(line);
  }
  DefaultAppendLine(line);
}

}  // namespace demo
