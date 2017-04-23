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

// This is a proof-of-concept example of how the framework could be used to
// limit the amount of effort expended by a computation. We do not expect this
// to be a widely-used use-case.

#include <chrono>
#include <iostream>
#include <list>
#include <string>
#include <thread>

#include "thread-capture.h"

using capture_thread::ThreadCapture;

// The base class just provides an interface for tracking/reporting resources.
class LimitEffort : public ThreadCapture<LimitEffort> {
 public:
  static bool ShouldContinue() {
    return GetCurrent() ? !GetCurrent()->LimitReached() : true;
  }

  static void Consume(int amount) {
    if (GetCurrent()) {
      GetCurrent()->DecrementResources(amount);
    }
  }

 protected:
  LimitEffort() = default;
  virtual ~LimitEffort() = default;

  virtual bool LimitReached() = 0;
  virtual void DecrementResources(int amount) {}

  // Don't add a ScopedCapture member to the base class, because that would make
  // it impossible to write an implementation that doesn't automatically capture
  // logging.
};

// This implementation imposes a time-based limit.
class LimitTime : public LimitEffort {
 public:
  explicit LimitTime(double seconds)
      : seconds_(seconds),
        start_time_(std::chrono::high_resolution_clock::now()),
        capture_to_(this) {}

  double ResourcesConsumed() const {
    const auto current_time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(current_time -
                                                                 start_time_)
               .count() /
           1000000.0;
  }

 protected:
  bool LimitReached() override { return ResourcesConsumed() > seconds_; }

 private:
  const double seconds_;
  const std::chrono::high_resolution_clock::time_point start_time_;
  const ScopedCapture capture_to_;
};

// This implementation imposes a counter-based limit.
class LimitCount : public LimitEffort {
 public:
  explicit LimitCount(int count) : count_(count), capture_to_(this) {}

  int ResourcesRemaining() const { return count_; }

 protected:
  bool LimitReached() override { return ResourcesRemaining() <= 0; }

  void DecrementResources(int amount) override { count_ -= amount; }

 private:
  int count_;
  const ScopedCapture capture_to_;
};

// Performs the actual work. It's important to remember that this function, and
// its callers all the way up, need to do something reasonable when stopping
// early. In this case, we're just counting. In other situations, you might have
// a partial result of a computation, which itself might be an acceptable result
// (e.g., an aborted search operation), or it might not be (e.g., a partially-
// completed matrix multiplication.)
void ResourceConsumingWorker() {
  for (int i = 0; i < 100 && LimitEffort::ShouldContinue(); ++i) {
    std::cerr << i << ' ';
    LimitEffort::Consume(i);
    std::this_thread::sleep_for(std::chrono::milliseconds(i));
  }
  std::cerr << std::endl;
}

void ProcessByTime() {
  LimitTime limit(1.0);
  ResourceConsumingWorker();
  std::cerr << "Resources consumed: " << limit.ResourcesConsumed() << std::endl;
}

void ProcessByCount() {
  LimitCount limit(500);
  ResourceConsumingWorker();
  std::cerr << "Resources remaining: " << limit.ResourcesRemaining()
            << std::endl;
}

int main() {
  std::cerr << "Process with time limit..." << std::endl;
  ProcessByTime();
  std::cerr << "Process with count limit..." << std::endl;
  ProcessByCount();
  std::cerr << "Process without limit..." << std::endl;
  ResourceConsumingWorker();
}
