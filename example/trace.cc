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
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "thread-capture.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

// (See threaded.cc for more info about threading.)
class TraceContext : public ThreadCapture<TraceContext> {
 public:
  TraceContext(std::string name)
      : name_(std::move(name)), cross_and_capture_to_(this) {}

  static std::vector<std::string> GetTrace() {
    std::vector<std::string> trace;
    if (GetCurrent()) {
      GetCurrent()->AppendTrace(&trace);
    }
    return trace;
  }

 private:
  // AutoThreadCrosser::Previous provides access to the next frame on the stack.
  // this can be used recursively to create a trace back to the first frame in
  // the current scope.

  void AppendTrace(std::vector<std::string>* trace) const {
    assert(trace);
    trace->push_back(name_);
    if (cross_and_capture_to_.Previous()) {
      cross_and_capture_to_.Previous()->AppendTrace(trace);
    }
  }

  const std::string name_;
  const AutoThreadCrosser cross_and_capture_to_;
};

void PrintTrace() {
  const auto trace = TraceContext::GetTrace();
  for (unsigned int i = 0; i < trace.size(); ++i) {
    std::cerr << "Frame " << i << ": " << trace[i] << std::endl;
  }
}

int main() {
  // Each important scope is labeled with a TraceContext. It's important to
  // remember that it must be *named*; otherwise, it will go out of scope
  // immediately, making it effectively a no-op.
  TraceContext trace_frame("main");

  // ThreadCrosser::WrapCall will capture the current frame and pass it on when
  // the callback is executed, regardless of where that happens.
  const auto execute = ThreadCrosser::WrapCall([] {
    TraceContext trace_frame("execute");
    PrintTrace();
  });

  std::thread worker_thread([execute] {
    // Not part of the trace when execute is called.
    TraceContext trace_frame("worker_thread");
    execute();
  });
  worker_thread.join();
}
