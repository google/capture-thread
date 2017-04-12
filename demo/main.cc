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
#include <thread>

#include "callback-queue.h"
#include "logging.h"
#include "tracing.h"

using capture_thread::ThreadCrosser;
using demo::CallbackQueue;
using demo::CaptureLogging;
using demo::Logging;
using demo::Tracing;

namespace {

void Compute(int value) {
  Tracing context(__func__);
  Logging::LogLine() << "Computing " << value;
  std::this_thread::sleep_for(std::chrono::milliseconds(value));
}

void QueueThread(int index, CallbackQueue* queue) {
  assert(queue);
  Tracing context(__func__);
  Logging::LogLine() << "Thread " << index << " starting";
  while (queue->PopAndCall()) {
  }
  Logging::LogLine() << "Thread " << index << " stopping";
}

std::unique_ptr<std::thread> NewThread(std::function<void()> callback) {
  return std::unique_ptr<std::thread>(
      new std::thread(ThreadCrosser::WrapCall(std::move(callback))));
}

}  // namespace

int main() {
  Tracing context(__func__);
  CallbackQueue queue(false /*active*/);

  for (int i = 0; i < 10; ++i) {
    queue.Push(std::bind(&Compute, i));
  }

  std::list<std::unique_ptr<std::thread>> threads;
  for (int i = 0; i < 3; ++i) {
    threads.emplace_back(NewThread(std::bind(&QueueThread, i, &queue)));
  }

  queue.Activate();
  queue.WaitUntilEmpty();
  queue.Terminate();

  for (const auto& thread : threads) {
    thread->join();
  }
}
