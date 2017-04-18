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

#ifndef LOGGING_H_
#define LOGGING_H_

#include <list>
#include <mutex>
#include <sstream>
#include <string>

#include "thread-capture.h"

namespace demo {

// Provides a text-logging mechanism. By default sends data to stderr. Use
// CaptureLogging to capture logged data.
class Logging : public capture_thread::ThreadCapture<Logging> {
 public:
  // Formats and logs a line. Operates as a std::ostream. For example:
  //
  //   Logging::LogLine() << "Log message.";
  class LogLine {
   public:
    LogLine();
    ~LogLine();

    template <class Type>
    LogLine& operator<<(Type value) {
      static_cast<std::ostream&>(output_) << value;
      return *this;
    }

   private:
    Logging* const capture_;
    std::ostringstream output_;
  };

 protected:
  Logging() = default;
  virtual ~Logging() = default;

  virtual void AppendLine(const std::string& line) = 0;

  static void DefaultAppendLine(const std::string& line);
};

// Captures lines logged with Logging while in scope.
class CaptureLogging : public Logging {
 public:
  CaptureLogging() : cross_and_capture_to_(this) {}

  // Returns a copy of all lines captured.
  std::list<std::string> CopyLines();

 protected:
  void AppendLine(const std::string& line) override;

 private:
  std::mutex data_lock_;
  std::list<std::string> lines_;
  const AutoThreadCrosser cross_and_capture_to_;
};

}  // namespace demo

#endif  // LOGGING_H_
