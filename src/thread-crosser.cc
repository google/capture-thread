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

#include "thread-crosser.h"

namespace capture_thread {

// static
std::function<void()> ThreadCrosser::WrapCall(std::function<void()> call) {
  return WrapFunction(call);
}

// static
std::function<void()> ThreadCrosser::WrapCall(std::function<void()> call,
                                              const ThreadCrosser* current) {
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

// static
thread_local ThreadCrosser* ThreadCrosser::current_(nullptr);

}  // namespace capture_thread
