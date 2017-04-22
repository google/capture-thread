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

#define CAPTURE_THREAD_EXPERIMENTAL
#include "thread-crosser.h"
#undef CAPTURE_THREAD_EXPERIMENTAL

#include <cassert>

namespace capture_thread {

namespace {

ThreadCrosser* global_override(nullptr);

}  // namespace

// static
std::function<void()> ThreadCrosser::WrapCall(std::function<void()> call) {
  const auto current = GetCurrent();
  if (call && current) {
    return current->WrapWithCrosser(WrapCallRec(std::move(call), current));
  } else {
    return call;
  }
}

// static
std::function<void()> ThreadCrosser::WrapCallRec(std::function<void()> call,
                                                 const ThreadCrosser* current) {
  if (current) {
    return WrapCallRec(current->WrapWithContext(std::move(call)),
                       current->Parent());
  } else {
    return call;
  }
}

ThreadCrosser::SetGlobalOverride::SetGlobalOverride()
    : current_(GetCurrent()) {
  assert(global_override == nullptr);
  global_override = current_;
}

ThreadCrosser::SetGlobalOverride::~SetGlobalOverride() {
  assert(global_override == current_);
  global_override = nullptr;
}

ThreadCrosser::UseGlobalOverride::UseGlobalOverride()
    : current_(global_override) {}

ThreadCrosser::UseGlobalOverride::~UseGlobalOverride() {
  assert(global_override == current_);
}

void ThreadCrosser::UseGlobalOverride::Call(std::function<void()> call) const {
  call = WrapCallRec(std::move(call), current_);
  if (call) {
    call();
  }
}

// static
thread_local ThreadCrosser* ThreadCrosser::current_(nullptr);

}  // namespace capture_thread
