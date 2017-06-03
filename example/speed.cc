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

constexpr int kRepetitions = 5;
constexpr int kMaxWraps = 4;
constexpr int kMaxScopes = 4;
constexpr int kIterations = 1000000;

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

void Execute(const std::function<int(int)>& function) {
  const auto start_time = GetCurrentTime();
  for (int i = 0; i < kIterations; ++i) {
    function(i);
  }
  const auto finish_time = GetCurrentTime();
  const double elapsed_ms = 1000. * (finish_time - start_time).count();
  std::cout << '\t' << elapsed_ms << '\t' << elapsed_ms / kIterations
            << std::endl;
}

void ExecuteWithWrapping(std::function<int(int)> function, int wraps) {
  if (wraps > 0) {
    ExecuteWithWrapping(ThreadCrosser::WrapFunction(function), wraps - 1);
  } else {
    Execute(function);
  }
}

void ExecuteWithScopesAndWrapping(int scopes, int wraps) {
  if (scopes > 0) {
    NoOp noop;
    ExecuteWithScopesAndWrapping(scopes - 1, wraps);
  } else {
    ExecuteWithWrapping([](int x) { return x; }, wraps);
  }
}

int main() {
  for (int i = 0; i < kRepetitions; ++i) {
    for (int wraps = 1; wraps < kMaxWraps + 1; ++wraps) {
      for (int scopes = 1; scopes < kMaxScopes + 1; ++scopes) {
        std::cout << i << '\t' << scopes << '\t' << wraps;
        ExecuteWithScopesAndWrapping(scopes, wraps);
      }
    }
  }
}
