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

#ifndef TRACING_H_
#define TRACING_H_

#include <list>
#include <mutex>
#include <sstream>
#include <string>

#include "thread-capture.h"

namespace demo {

class Formatter {
 public:
  std::string String() const { return output_.str(); }

  template <class Type>
  Formatter& operator<<(Type value) {
    static_cast<std::ostream&>(output_) << value;
    return *this;
  }

 private:
  std::ostringstream output_;
};

class Tracing : public capture_thread::ThreadCapture<Tracing> {
 public:
  explicit Tracing(std::string name)
      : name_(std::move(name)), cross_and_capture_to_(this) {}

  static std::string GetContext();

 private:
  const std::string& name() const { return name_; }

  static void ReverseTrace(const Tracing* tracer, Formatter* formatter);

  const std::string name_;
  const AutoThreadCrosser cross_and_capture_to_;
};

}  // namespace demo

#endif  // TRACING_H_
