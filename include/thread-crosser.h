#ifndef THREAD_CROSSER_H_
#define THREAD_CROSSER_H_

#include <functional>

#include "thread-capture.h"

class ThreadCrosser : public ThreadCapture<ThreadCrosser> {
 public:
  static std::function<void()> WrapCall(std::function<void()> call);

 protected:
  ThreadCrosser() : parent_(GetCurrent()), capture_to_(this) {}
  virtual ~ThreadCrosser() = default;

  virtual std::function<void()> WrapFunction(
      std::function<void()> call) const = 0;

 private:
  ThreadCrosser(const ThreadCrosser&) = delete;
  ThreadCrosser(ThreadCrosser&&) = delete;
  ThreadCrosser& operator =(const ThreadCrosser&) = delete;
  ThreadCrosser& operator =(ThreadCrosser&&) = delete;
  void* operator new(std::size_t size) = delete;

  static std::function<void()> WrapCallRec(
      std::function<void()> call, const ThreadCrosser* current);

  // parent_ must stay before capture_to_.
  ThreadCrosser* const parent_;
  const ScopedCapture capture_to_;
  const ThreadCrosser::ThreadBridge bridge_;
};

template <class Type>
class AutoThreadCrosser : public ThreadCrosser, public ThreadCapture<Type> {
 public:
  AutoThreadCrosser(Type* logger) : capture_to_(logger) {}

 protected:
  std::function<void()> WrapFunction(
      std::function<void()> call) const override {
    if (call) {
      return [this, call] {
        const typename ThreadCapture<Type>::CrossThreads logger(bridge_);
        call();
      };
    } else {
      return call;
    }
  }

 private:
  const typename ThreadCapture<Type>::ScopedCapture capture_to_;
  // bridge_ must stay after capture_to_.
  const typename ThreadCapture<Type>::ThreadBridge bridge_;
};

#endif  // THREAD_CROSSER_H_
