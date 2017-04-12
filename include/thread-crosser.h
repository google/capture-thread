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

#ifndef THREAD_CROSSER_H_
#define THREAD_CROSSER_H_

#include <functional>

#include "thread-capture.h"

namespace capture_thread {

// Automatically makes an object shared within a single thread to be shared with
// another thread. Use AutoThreadCrosser to add the functionality to an object,
// then use ThreadCrosser::WrapCall to enable sharing for a specific callback.
class ThreadCrosser : public ThreadCapture<ThreadCrosser> {
 public:
  // NOTE: The return of WrapCall (and its copies) must never outlive the scope
  // that WrapCall was called in, even if the functions are passed to other
  // threads. This is because WrapCall captures elements of the stack.
  static std::function<void()> WrapCall(std::function<void()> call);

 protected:
  ThreadCrosser() : capture_to_(this) {}
  virtual ~ThreadCrosser() = default;

  virtual std::function<void()> WrapWithContext(
      std::function<void()> call) const = 0;

 private:
  ThreadCrosser(const ThreadCrosser&) = delete;
  ThreadCrosser(ThreadCrosser&&) = delete;
  ThreadCrosser& operator=(const ThreadCrosser&) = delete;
  ThreadCrosser& operator=(ThreadCrosser&&) = delete;
  void* operator new(std::size_t size) = delete;

  static std::function<void()> WrapCallRec(std::function<void()> call,
                                           const ThreadCrosser* current);

  const ScopedCapture capture_to_;
};

// Enables cross-thread sharing for the given type. Use this class instead of
// ThreadCapture<Type>::ScopedCapture when defining Type.
// NOTE: Type must be derived from ThreadCapture<Type> for visibility reasons;
// otherwise, you will get a compiler error. This is to ensure that you don't
// accidentally use the wrong Type parameter, which would otherwise be possible
// since this is a public class.
template <class Type>
class AutoThreadCrosser : public ThreadCrosser {
 public:
  AutoThreadCrosser(Type* capture) : capture_to_(capture) {
    TypeMustInheritFromThreadCapture(capture);
  }

  Type* Previous() const { return capture_to_.Previous(); }

 protected:
  std::function<void()> WrapWithContext(
      std::function<void()> call) const override {
    if (call) {
      return [this, call] {
        const typename Type::CrossThreads capture(capture_to_);
        call();
      };
    } else {
      return call;
    }
  }

 private:
  static constexpr void TypeMustInheritFromThreadCapture(
      const ThreadCapture<Type>* capture) {}

  // Type must inherit from ThreadCapture<Type>.
  const typename Type::ScopedCapture capture_to_;
};

}  // namespace capture_thread

#endif  // THREAD_CROSSER_H_
