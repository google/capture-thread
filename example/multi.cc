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

// This example demonstrates how ThreadCrosser::WrapCall will automatically
// capture all ThreadCapture instances currently in scope, as long as the latter
// each have an AutoThreadCrosser as a member. No additional work is required
// for crossing threads if you need to use multiple types of ThreadCapture.

#include <iostream>
#include <string>
#include <thread>

#include "thread-capture.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

// Example implementation, 1 of 2.
class LogTypeOne : public ThreadCapture<LogTypeOne> {
 public:
  explicit LogTypeOne(const std::string& value)
      : value_(value), cross_and_capture_to_(this) {}

  static void Show() {
    if (GetCurrent()) {
      GetCurrent()->ShowValue();
    }
  }

 private:
  void ShowValue() const { std::cout << value_ << std::endl; }

  const std::string value_;
  const AutoThreadCrosser cross_and_capture_to_;
};

// Example implementation, 2 of 2.
class LogTypeTwo : public ThreadCapture<LogTypeTwo> {
 public:
  explicit LogTypeTwo(const std::string& value)
      : value_(value), cross_and_capture_to_(this) {}

  static void Show() {
    if (GetCurrent()) {
      GetCurrent()->ShowValue();
    }
  }

 private:
  void ShowValue() const { std::cout << value_ << std::endl; }

  const std::string value_;
  const AutoThreadCrosser cross_and_capture_to_;
};

void WorkerThread() {
  LogTypeOne::Show();
  LogTypeTwo::Show();
}

int main() {
  const LogTypeOne superceded_by_type1("should not print");
  const LogTypeOne type1("type1 was captured");
  const LogTypeTwo type2("type2 was captured");

  // It doesn't matter how many implementations are in scope; all are captured
  // with a single call to ThreadCrosser::WrapCall. Only the most-recent of each
  // ThreadCapture type will be used! (See threaded.cc.)
  std::thread worker1(ThreadCrosser::WrapCall(&WorkerThread));
  worker1.join();

  // The same applies to ThreadCrosser::SetOverride::Call. (See framework.cc.)
  const ThreadCrosser::SetOverride override_point;
  std::thread worker2(
      [&override_point] { override_point.Call(&WorkerThread); });
  worker2.join();
}
