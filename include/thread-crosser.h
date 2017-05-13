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
#include <type_traits>
#include <cassert>

namespace capture_thread {

// Automatically makes an object shared within a single thread to be shared with
// another thread. Use ThreadCapture::AutoThreadCrosser to add the functionality
// to an object, then use ThreadCrosser::WrapCall or ThreadCrosser::WrapFunction
// to enable sharing for a specific callback.
class ThreadCrosser {
 public:
  // Wraps a simple callback with the current scope. The return must *never*
  // outlive the scope that it was created in.
  static std::function<void()> WrapCall(std::function<void()> call);

  // Wraps a general function with the current scope. The return must *never*
  // outlive the scope that it was created in.
  template <class Return, class... Args>
  static std::function<Return(Args...)> WrapFunction(
      std::function<Return(Args...)> function);

 private:
  ThreadCrosser(const ThreadCrosser&) = delete;
  ThreadCrosser(ThreadCrosser&&) = delete;
  ThreadCrosser& operator=(const ThreadCrosser&) = delete;
  ThreadCrosser& operator=(ThreadCrosser&&) = delete;
  void* operator new(std::size_t size) = delete;

  ThreadCrosser() = default;
  virtual ~ThreadCrosser() = default;

  // Allows nested thread-crossing (i.e., crossing threads again within a
  // thread) to work properly.
  virtual std::function<void()> WrapWithCrosser(
      std::function<void()> call) const = 0;

  // Captures a specific ThreadCapture instance and binds it to the function.
  virtual std::function<void()> WrapWithContext(
      std::function<void()> call) const = 0;

  virtual ThreadCrosser* Parent() const = 0;

  class ScopedCrosser;

  // Makes a captured ThreadCrosser available within the current thread. This is
  // only to allow thread-crossing to happen again within that thread.
  class DelegateCrosser {
   public:
    explicit inline DelegateCrosser(const ScopedCrosser& crosser)
        : parent_(GetCurrent()) {
      SetCurrent(crosser.current_);
    }

    inline ~DelegateCrosser() { SetCurrent(parent_); }

   private:
    DelegateCrosser(const DelegateCrosser&) = delete;
    DelegateCrosser(DelegateCrosser&&) = delete;
    DelegateCrosser& operator=(const DelegateCrosser&) = delete;
    DelegateCrosser& operator=(DelegateCrosser&&) = delete;
    void* operator new(std::size_t size) = delete;

    ThreadCrosser* const parent_;
  };

  // Captures the ThreadCrosser so it can be used by DelegateCrosser, which will
  // make it available in another thread.
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

    friend class DelegateCrosser;
    ThreadCrosser* const parent_;
    ThreadCrosser* const current_;
  };

  // AutoMove (and its specializations) ensure that argument-passing when
  // calling a std::function doesn't make copies when it would be inappropriate,
  // e.g., when Type cannot be copied or moved. This is a class instead of a
  // function so that Type can be made explicit, which is necessary for type-
  // checking to work.

  // Handles pass-by-value.
  template <class Type>
  struct AutoMove {
    static Type&& Pass(Type& value) { return std::move(value); }
  };

  // Handles pass-by-reference.
  template <class Type>
  struct AutoMove<Type&> {
    static Type& Pass(Type& value) { return value; }
  };

  // AutoCall (and its specializations) ensure that return values are not
  // inappropriately copied, e.g., when returning by reference, or when the
  // return type is void. This is a class instead of a function so that the
  // types can be made explicit, which is necessary for type-checking to work.

  // Handles return-by-value.
  template <class Return, class... Args>
  struct AutoCall {
    static Return Execute(ThreadCrosser* current,
                          const std::function<Return(Args...)>& function,
                          Args... args) {
      assert(function);
      typename std::remove_cv<Return>::type value = Return();
      ThreadCrosser::WrapEntireScope([&value, &function, &args...] {
        value = function(AutoMove<Args>::Pass(args)...);
      }, current)();
      return value;
    }
  };

  // Handles return-by-reference.
  template <class Return, class... Args>
  struct AutoCall<Return&, Args...> {
    static Return& Execute(ThreadCrosser* current,
                           const std::function<Return&(Args...)>& function,
                           Args... args) {
      assert(function);
      Return* value(nullptr);
      ThreadCrosser::WrapEntireScope([&value, &function, &args...] {
        value = &function(AutoMove<Args>::Pass(args)...);
      }, current)();
      assert(value);
      return *value;
    }
  };

  // Handles void return type.
  template <class... Args>
  struct AutoCall<void, Args...> {
    static void Execute(ThreadCrosser* current,
                        const std::function<void(Args...)>& function,
                        Args... args) {
      assert(function);
      ThreadCrosser::WrapEntireScope([&function, &args...] {
        function(AutoMove<Args>::Pass(args)...);
      }, current)();
    }
  };

  // Wraps the function with the specified ThreadCrosser's entire scope.
  static std::function<void()> WrapEntireScope(std::function<void()> call,
                                               const ThreadCrosser* current);

  // Recursively wraps the function with the ThreadCapture instances that are
  // currently in scope.
  static std::function<void()> WrapContextRec(std::function<void()> call,
                                              const ThreadCrosser* current);

  static inline ThreadCrosser* GetCurrent() { return current_; }
  static inline void SetCurrent(ThreadCrosser* value) { current_ = value; }

  static thread_local ThreadCrosser* current_;

  template <class Type>
  friend class ThreadCapture;
};

template <class Return, class... Args>
std::function<Return(Args...)> ThreadCrosser::WrapFunction(
    std::function<Return(Args...)> function) {
  if (function) {
    const auto current = GetCurrent();
    return [current, function](Args... args) -> Return {
      return AutoCall<Return, Args...>::Execute(
          current, std::move(function), AutoMove<Args>::Pass(args)...);
    };
  } else {
    return function;
  }
}

}  // namespace capture_thread

#endif  // THREAD_CROSSER_H_
