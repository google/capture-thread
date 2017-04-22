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

namespace capture_thread {

// Automatically makes an object shared within a single thread to be shared with
// another thread. Use ThreadCapture::AutoThreadCrosser to add the functionality
// to an object, then use ThreadCrosser::WrapCall to enable sharing for a
// specific callback.
class ThreadCrosser {
 public:
  // NOTE: The return of WrapCall (and its copies) must never outlive the scope
  // that WrapCall was called in, even if the functions are passed to other
  // threads. This is because WrapCall captures elements of the stack.
  static std::function<void()> WrapCall(std::function<void()> call);

#ifdef CAPTURE_THREAD_EXPERIMENTAL

  // Sets a global override based on the current scope. Use this *only* in
  // situations where you cannot pass a function that has been wrapped with
  // WrapCall to a thread. (For example, if you are using another framework that
  // manages its own threads, and cannot pass a std::function from the main
  // thread to the worker thread.) Instantiate this class to capture the current
  // scope, and use UseGlobalOverride to wrap calls.
  // NOTE: Use with caution! This class is not thread-safe and should therefore
  // be instantiated no more than once, no matter what.
  class SetGlobalOverride {
   public:
    SetGlobalOverride();
    ~SetGlobalOverride();

   private:
    SetGlobalOverride(const SetGlobalOverride&) = delete;
    SetGlobalOverride(SetGlobalOverride&&) = delete;
    SetGlobalOverride& operator=(const SetGlobalOverride&) = delete;
    SetGlobalOverride& operator=(SetGlobalOverride&&) = delete;
    void* operator new(std::size_t size) = delete;

    ThreadCrosser* const current_;
  };

  // Allows the current thread to access the scope override set in the main
  // thread with SetGlobalOverride. Even though SetGlobalOverride isn't thread-
  // safe, this class is; *however*, instances of this class must never
  // outlive any SetGlobalOverride instance.
  class UseGlobalOverride {
   public:
    UseGlobalOverride();
    ~UseGlobalOverride();

    // Wraps the call with the current global override, then calls it.
    void Call(std::function<void()> call) const;

   private:
    UseGlobalOverride(const UseGlobalOverride&) = delete;
    UseGlobalOverride(UseGlobalOverride&&) = delete;
    UseGlobalOverride& operator=(const UseGlobalOverride&) = delete;
    UseGlobalOverride& operator=(UseGlobalOverride&&) = delete;
    void* operator new(std::size_t size) = delete;

    ThreadCrosser* const current_;
  };

#endif  // CAPTURE_THREAD_EXPERIMENTAL

 private:
  ThreadCrosser(const ThreadCrosser&) = delete;
  ThreadCrosser(ThreadCrosser&&) = delete;
  ThreadCrosser& operator=(const ThreadCrosser&) = delete;
  ThreadCrosser& operator=(ThreadCrosser&&) = delete;
  void* operator new(std::size_t size) = delete;

  ThreadCrosser() = default;
  virtual ~ThreadCrosser() = default;

  virtual std::function<void()> WrapWithCrosser(
      std::function<void()> call) const = 0;

  virtual std::function<void()> WrapWithContext(
      std::function<void()> call) const = 0;

  virtual ThreadCrosser* Parent() const = 0;

  class ScopedOverride;

  class ScopedCrosser {
   public:
    explicit inline ScopedCrosser(ThreadCrosser* capture)
        : parent_(GetCurrent()), current_(capture) {
      SetCurrent(capture);
    }

    inline ~ScopedCrosser() { SetCurrent(Parent()); }

    inline ThreadCrosser* Parent() const { return parent_; }

   private:
    ScopedCrosser(const ScopedCrosser&) = delete;
    ScopedCrosser(ScopedCrosser&&) = delete;
    ScopedCrosser& operator=(const ScopedCrosser&) = delete;
    ScopedCrosser& operator=(ScopedCrosser&&) = delete;
    void* operator new(std::size_t size) = delete;

    friend class ScopedOverride;
    ThreadCrosser* const parent_;
    ThreadCrosser* const current_;
  };

  class ScopedOverride {
   public:
    explicit inline ScopedOverride(const ScopedCrosser& crosser)
        : parent_(GetCurrent()) {
      SetCurrent(crosser.current_);
    }

    inline ~ScopedOverride() { SetCurrent(parent_); }

   private:
    ScopedOverride(const ScopedOverride&) = delete;
    ScopedOverride(ScopedOverride&&) = delete;
    ScopedOverride& operator=(const ScopedOverride&) = delete;
    ScopedOverride& operator=(ScopedOverride&&) = delete;
    void* operator new(std::size_t size) = delete;

    ThreadCrosser* const parent_;
  };

  static std::function<void()> WrapCallRec(std::function<void()> call,
                                           const ThreadCrosser* current);

  static inline ThreadCrosser* GetCurrent() { return current_; }
  static inline void SetCurrent(ThreadCrosser* value) { current_ = value; }

  static thread_local ThreadCrosser* current_;

  template <class Type>
  friend class ThreadCapture;
};

}  // namespace capture_thread

#endif  // THREAD_CROSSER_H_
