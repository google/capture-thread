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

// This is a proof-of-concept example of how you might rate-throttle worker
// threads using a shared timer. For example, you might multithread an operation
// that repeatedly polls a resource where each poll operation is comparatively
// slow, but when combined, the threads would exceed some desired rate limit.

#include <chrono>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "thread-capture.h"
#include "thread-crosser.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

// Base class for throttlers that limit the rate of processing.
class RateThrottler : public ThreadCapture<RateThrottler> {
 public:
  static void Wait() {
    if (GetCurrent()) {
      GetCurrent()->WaitForNextEvent();
    }
  }

 protected:
  RateThrottler() = default;
  virtual ~RateThrottler() = default;

  virtual void WaitForNextEvent() = 0;
};

// Limits the processing rate based on a shared internal timer.
class SharedThrottler : public RateThrottler {
 public:
  explicit SharedThrottler(double seconds_between_events)
      : seconds_between_events_(seconds_between_events),
        last_time_(GetCurrentTime() - seconds_between_events_),
        cross_and_capture_to_(this) {}

 protected:
  void WaitForNextEvent() override {
    std::lock_guard<std::mutex> lock(time_lock_);
    const auto current_time = GetCurrentTime();
    std::this_thread::sleep_for(seconds_between_events_ -
                                (current_time - last_time_));
    last_time_ = GetCurrentTime();
  }

 private:
  static std::chrono::duration<double> GetCurrentTime() {
    return std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
  }

  const std::chrono::duration<double> seconds_between_events_;
  std::mutex time_lock_;
  std::chrono::duration<double> last_time_;
  const AutoThreadCrosser cross_and_capture_to_;
};

// This represents a worker thread that is expected to not exceed the rate limit
// on its own, but might exceed that rate in combination with other workers.
void Worker(int number) {
  for (int i = 0; i < 5; ++i) {
    RateThrottler::Wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(125));
    std::ostringstream formatted;
    formatted << "Thread #" << number << ": " << i << std::endl;
    std::cerr << formatted.str();
  }
}

// The work is parallelized because each individual operation is slow.
void Execute() {
  std::list<std::unique_ptr<std::thread>> threads;
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back(
        new std::thread(ThreadCrosser::WrapCall(std::bind(&Worker, i))));
  }
  for (const auto& thread : threads) {
    thread->join();
  }
}

int main() {
  {
    std::cerr << "Using kMean of 100ms" << std::endl;
    SharedThrottler throttler(0.1);
    Execute();
  }
  std::cerr << "Without throttling" << std::endl;
  Execute();
}
