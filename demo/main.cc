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

#include <cassert>
#include <chrono>
#include <list>
#include <memory>
#include <sstream>
#include <thread>

#include "callback-queue.h"
#include "logging.h"
#include "tracing.h"

using capture_thread::ThreadCrosser;
using demo::CallbackQueue;
using demo::CaptureLogging;
using demo::Formatter;
using demo::Logging;
using demo::Tracing;

namespace {

// A unit of computation that can be parallelized.
void Compute(int value) {
  Tracing context(__func__);
  Logging::LogLine() << "Computing " << value;
  std::this_thread::sleep_for(std::chrono::milliseconds(value));
}

// A worker thread that executes whatever is in the queue.
void QueueThread(int index, CallbackQueue* queue) {
  assert(queue);
  Tracing context((Formatter() << __func__ << '[' << index << ']').String());
  Logging::LogLine() << "Thread starting";
  while (queue->PopAndCall()) {
  }
  Logging::LogLine() << "Thread stopping";
}

// Worker threads don't need to be wrapped with ThreadCrosser::WrapCall if they
// are just executing callbacks from a queue, but it can be helpful, e.g., for
// tracing purposes.
std::unique_ptr<std::thread> NewThread(std::function<void()> callback) {
  return std::unique_ptr<std::thread>(
      new std::thread(ThreadCrosser::WrapCall(std::move(callback))));
}

}  // namespace

int main() {
  Tracing context(__func__);

  // Queue for passing work from the main thread to the worker threads. Created
  // in a paused state.
  CallbackQueue queue(false /*active*/);

  for (int i = 0; i < 10; ++i) {
    // One callback per unit of work that can be parallelized.
    queue.Push(std::bind(&Compute, i));
  }

  std::list<std::unique_ptr<std::thread>> threads;
  for (int i = 0; i < 3; ++i) {
    // An arbitrary number of threads.
    threads.emplace_back(NewThread(std::bind(&QueueThread, i, &queue)));
  }

  // Perform the computations.

  queue.Activate();
  queue.WaitUntilEmpty();
  queue.Terminate();

  for (const auto& thread : threads) {
    thread->join();
  }
}
