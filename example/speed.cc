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

// This is a simple (and noisy) test of the overhead required for each
// ThreadCapture captured by ThreadCrosser::WrapCall.

#include <chrono>
#include <functional>
#include <iostream>
#include <string>

#include "thread-capture.h"
#include "thread-crosser.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

class NoOp : public ThreadCapture<NoOp> {
 public:
  NoOp() : cross_and_capture_to_(this) {}

 private:
  const AutoThreadCrosser cross_and_capture_to_;
};

static std::chrono::duration<double> GetCurrentTime() {
  return std::chrono::duration_cast<std::chrono::duration<double>>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
}

void TimeIterations(const std::string& prefix,
                    const std::function<void()>& call, int count) {
  const auto start_time = GetCurrentTime();
  for (int i = 0; i < count; ++i) {
    call();
  }
  const auto finish_time = GetCurrentTime();
  const double elapsed_ms = 1000. * (finish_time - start_time).count();
  std::cerr << prefix << ":\t" << elapsed_ms << "\t(" << elapsed_ms / count
            << ")" << std::endl;
}

int main() {
  constexpr int iterations = 10000000;
  std::function<void()> empty([] {});
  TimeIterations("0", ThreadCrosser::WrapCall(empty), iterations);
  {
    NoOp noop1;
    TimeIterations("1", ThreadCrosser::WrapCall(empty), iterations);
    {
      NoOp noop2;
      TimeIterations("2", ThreadCrosser::WrapCall(empty), iterations);
      {
        NoOp noop3;
        TimeIterations("3", ThreadCrosser::WrapCall(empty), iterations);
        {
          NoOp noop4;
          TimeIterations("4", ThreadCrosser::WrapCall(empty), iterations);
        }
        TimeIterations("3", ThreadCrosser::WrapCall(empty), iterations);
      }
      TimeIterations("2", ThreadCrosser::WrapCall(empty), iterations);
    }
    TimeIterations("1", ThreadCrosser::WrapCall(empty), iterations);
  }
  TimeIterations("0", ThreadCrosser::WrapCall(empty), iterations);
}
