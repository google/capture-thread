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

#include "thread-crosser.h"

namespace capture_thread {

// static
std::function<void()> ThreadCrosser::WrapCall(std::function<void()> call) {
  if (call && GetCurrent()) {
    call = WrapCallRec(std::move(call), GetCurrent());
    const ThreadCrosser::ThreadBridge* const bridge = &GetCurrent()->bridge_;
    return [bridge, call] {
      const ThreadCrosser::CrossThreads logger(*bridge);
      call();
    };
  } else {
    return call;
  }
}

// static
std::function<void()> ThreadCrosser::WrapCallRec(
    std::function<void()> call, const ThreadCrosser* current) {
  if (current) {
    return WrapCallRec(
        current->WrapFunction(std::move(call)), current->parent_);
  } else {
    return call;
  }
}

}  // namespace capture_thread
