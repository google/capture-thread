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

#include "callback-queue.h"

namespace capture_thread {
namespace testing {

void CallbackQueue::Push(std::function<void()> callback) {
  std::lock_guard<std::mutex> lock(queue_lock_);
  if (!terminated_) {
    queue_.push(std::move(callback));
    condition_.notify_all();
  }
}

bool CallbackQueue::PopAndCall() {
  std::unique_lock<std::mutex> lock(queue_lock_);
  while (!terminated_ && (!active_ || queue_.empty())) {
    condition_.wait(lock);
  }
  if (terminated_) {
    return false;
  } else {
    const auto callback = queue_.front();
    ++pending_;
    queue_.pop();
    lock.unlock();
    if (callback) {
      callback();
    }
    lock.lock();
    --pending_;
    condition_.notify_all();
    return true;
  }
}

void CallbackQueue::WaitUntilEmpty() {
  std::unique_lock<std::mutex> lock(queue_lock_);
  while (!terminated_ && (!queue_.empty() || pending_ > 0)) {
    condition_.wait(lock);
  }
}

void CallbackQueue::Terminate() {
  std::lock_guard<std::mutex> lock(queue_lock_);
  terminated_ = true;
  condition_.notify_all();
}

void CallbackQueue::Activate() {
  std::lock_guard<std::mutex> lock(queue_lock_);
  active_ = true;
  condition_.notify_all();
}

}  // namespace testing
}  // namespace capture_thread
