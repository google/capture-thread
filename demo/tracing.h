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

class Tracing : public capture_thread::ThreadCapture<Tracing> {
 public:
  Tracing(const std::string& name)
      : cross_and_capture_to_(this),
        name_(JoinWithPrevious(name, cross_and_capture_to_.Previous())) {}

  static std::string GetContext();

 private:
  const std::string& name() const { return name_; }

  static std::string JoinWithPrevious(const std::string& name,
                                      const Tracing* previous);

  const AutoThreadCrosser cross_and_capture_to_;
  const std::string name_;
};

}  // namespace demo

#endif  // TRACING_H_
