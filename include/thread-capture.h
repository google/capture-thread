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

#ifndef THREAD_CAPTURE_H_
#define THREAD_CAPTURE_H_

#include <cassert>

#include "thread-crosser.h"
#include "thread-local.h"

namespace capture_thread {

// Base class for instrumentation classes that need to be made available within
// a single thread, and possibly shared across threads. Derive a new class Type
// from ThreadCapture<Type> to define a new instrumentation type. Scoping is
// managed by adding a ScopedCapture *or* an AutoThreadCrosser member to
// instantiable implementations of the instrumentation. (If Type is abstract,
// add the member to the instantiable subclasses.) See ThreadCrosser for info
// about *automatic* sharing across threads, and ThreadBridge (below) for info
// about *manually* sharing across threads.
template <class Type>
class ThreadCapture {
 protected:
  class CrossThreads;

  // Manages scoping of instrumentation within a single thread. Use ThreadBridge
  // + CrossThreads to share instrumentation across threads. Alternatively, use
  // AutoThreadCrosser *instead* of ScopedCapture to allow ThreadCrosser to
  // handle this automatically.
  class ScopedCapture {
   public:
    explicit inline ScopedCapture(Type* capture)
        : previous_(GetCurrent()), current_(capture) {
      SetCurrent(capture);
    }

    inline ~ScopedCapture() { SetCurrent(Previous()); }

    inline Type* Previous() const { return previous_; }

   private:
    ScopedCapture(const ScopedCapture&) = delete;
    ScopedCapture(ScopedCapture&&) = delete;
    ScopedCapture& operator=(const ScopedCapture&) = delete;
    ScopedCapture& operator=(ScopedCapture&&) = delete;
    void* operator new(std::size_t size) = delete;

    friend class CrossThreads;
    Type* const previous_;
    Type* const current_;
  };

  // Creates a bridge point for sharing a single type of instrumentation between
  // threads. Create this in the main thread, then enable crossing in the worker
  // thread with CrossThreads. This is only needed if the instrumentation uses
  // ScopedCapture instead of AutoThreadCrosser.
  //
  // NOTE: This isn't visible by default. If you want to make this functionality
  // available for a specific instrumentation class, add the following with
  // public visibility within the instrumentation class:
  //
  //   using ThreadCapture<Type>::ThreadBridge;
  class ThreadBridge {
   public:
    inline ThreadBridge() : capture_(GetCurrent()) {}

   private:
    ThreadBridge(const ThreadBridge&) = delete;
    ThreadBridge(ThreadBridge&&) = delete;
    ThreadBridge& operator=(const ThreadBridge&) = delete;
    ThreadBridge& operator=(ThreadBridge&&) = delete;
    void* operator new(std::size_t size) = delete;

    friend class CrossThreads;
    Type* const capture_;
  };

  class AutoThreadCrosser;

  // Connects a worker thread to the main thread via a ThreadBridge for manually
  // sharing of a single type of instrumentation.
  //
  // NOTE: This isn't visible by default. If you want to make this functionality
  // available for a specific instrumentation class, add the following with
  // public visibility within the instrumentation class:
  //
  //   using ThreadCapture<Type>::CrossThreads;
  class CrossThreads {
   public:
    explicit inline CrossThreads(const ThreadBridge& bridge)
        : previous_(GetCurrent()) {
      SetCurrent(bridge.capture_);
    }

    inline ~CrossThreads() { SetCurrent(previous_); }

   private:
    explicit inline CrossThreads(const ScopedCapture& capture)
        : previous_(GetCurrent()) {
      SetCurrent(capture.current_);
    }

    CrossThreads(const CrossThreads&) = delete;
    CrossThreads(CrossThreads&&) = delete;
    CrossThreads& operator=(const CrossThreads&) = delete;
    CrossThreads& operator=(CrossThreads&&) = delete;
    void* operator new(std::size_t size) = delete;

    friend class AutoThreadCrosser;
    Type* const previous_;
  };

  // Enables automatic crossing of threads for a specific instrumentation type
  // when ThreadCrosser::WrapCall or ThreadCrosser::WrapFunction are used. Add
  // this as a member of the implementation that you want to automatically
  // share. Use ScopedCapture *instead* if you don't want ThreadCrosser to
  // automatically share instrumentation.
  class AutoThreadCrosser : public ThreadCrosser {
   public:
    AutoThreadCrosser(Type* capture)
        : cross_with_(this), capture_to_(capture) {}

    inline Type* Previous() const { return capture_to_.Previous(); }

   private:
    AutoThreadCrosser(const AutoThreadCrosser&) = delete;
    AutoThreadCrosser(AutoThreadCrosser&&) = delete;
    AutoThreadCrosser& operator=(const AutoThreadCrosser&) = delete;
    AutoThreadCrosser& operator=(AutoThreadCrosser&&) = delete;
    void* operator new(std::size_t size) = delete;

    void FindTopAndCall(const std::function<void()>& call,
                        const ReverseScope& reverse_scope) const final;

    void ReconstructContextAndCall(
        const std::function<void()>& call,
        const ReverseScope& reverse_scope) const final;

    const ScopedCrosser cross_with_;
    const ScopedCapture capture_to_;
  };

  ThreadCapture() = default;
  virtual ~ThreadCapture() = default;

  // Gets the most-recent object from the stack of the current thread.
  static inline Type* GetCurrent() {
    return ThreadLocal<Type>::GetCurrentThreadValue();
  }

 private:
  ThreadCapture(const ThreadCapture&) = delete;
  ThreadCapture(ThreadCapture&&) = delete;
  ThreadCapture& operator=(const ThreadCapture&) = delete;
  ThreadCapture& operator=(ThreadCapture&&) = delete;
  void* operator new(std::size_t size) = delete;

  static inline void SetCurrent(Type* value) {
    ThreadLocal<Type>::SetCurrentThreadValue(value);
  }
};

template <class Type>
thread_local Type* ThreadCapture<Type>::current_(nullptr);

template <class Type>
void ThreadCapture<Type>::AutoThreadCrosser::FindTopAndCall(
    const std::function<void()>& call,
    const ReverseScope& reverse_scope) const {
  if (cross_with_.Parent()) {
    cross_with_.Parent()->FindTopAndCall(
        call, {cross_with_.Parent(), &reverse_scope});
  } else {
    ReconstructContextAndCall(call, reverse_scope);
  }
}

template <class Type>
void ThreadCapture<Type>::AutoThreadCrosser::ReconstructContextAndCall(
    const std::function<void()>& call,
    const ReverseScope& reverse_scope) const {
  // Makes the ThreadCapture available in this scope so that it overrides the
  // default behavior of instrumentation calls made in call.
  const CrossThreads capture(capture_to_);
  if (reverse_scope.previous) {
    const auto current = reverse_scope.previous->current;
    assert(current);
    if (current) {
      current->ReconstructContextAndCall(call, *reverse_scope.previous);
    }
  } else {
    // Makes the ThreadCrosser available in this scope so that call itself can
    // cross threads again.
    const DelegateCrosser crosser(cross_with_);
    assert(call);
    if (call) {
      call();
    }
  }
}

}  // namespace capture_thread

#endif  // THREAD_CAPTURE_H_
