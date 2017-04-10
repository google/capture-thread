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

namespace capture_thread {

template<class Type>
class ThreadCapture {
 private:
  using ValueType = Type*;

 public:
  class CrossThreads;

  class ThreadBridge {
   public:
    inline ThreadBridge() : capture_(GetCurrent()) {}

   private:
    ThreadBridge(const ThreadBridge&) = delete;
    ThreadBridge(ThreadBridge&&) = delete;
    ThreadBridge& operator =(const ThreadBridge&) = delete;
    ThreadBridge& operator =(ThreadBridge&&) = delete;
    void* operator new(std::size_t size) = delete;

    friend class CrossThreads;
    const ValueType capture_;
  };

  class CrossThreads {
   public:
    explicit inline CrossThreads(const ThreadBridge& bridge) :
        previous_(GetCurrent()) {
      SetCurrent(bridge.capture_);
    }

    inline ~CrossThreads() { SetCurrent(previous_); }

   private:
    CrossThreads(const CrossThreads&) = delete;
    CrossThreads(CrossThreads&&) = delete;
    CrossThreads& operator =(const CrossThreads&) = delete;
    CrossThreads& operator =(CrossThreads&&) = delete;
    void* operator new(std::size_t size) = delete;

    const ValueType previous_;
  };

 protected:
  ThreadCapture() = default;
  ~ThreadCapture() = default;

  static inline ValueType GetCurrent() { return current_; }

  class ScopedCapture {
   public:
    explicit inline ScopedCapture(ValueType capture) : previous_(GetCurrent()) {
      SetCurrent(capture);
    }

    inline ~ScopedCapture() { SetCurrent(previous_); }

   private:
    ScopedCapture(const ScopedCapture&) = delete;
    ScopedCapture(ScopedCapture&&) = delete;
    ScopedCapture& operator =(const ScopedCapture&) = delete;
    ScopedCapture& operator =(ScopedCapture&&) = delete;
    void* operator new(std::size_t size) = delete;

    const ValueType previous_;
  };

 private:
  ThreadCapture(const ThreadCapture&) = delete;
  ThreadCapture(ThreadCapture&&) = delete;
  ThreadCapture& operator =(const ThreadCapture&) = delete;
  ThreadCapture& operator =(ThreadCapture&&) = delete;

  static inline void SetCurrent(ValueType value) {
    current_ = value;
  }

  static thread_local ValueType current_;
};

template <class Type>
thread_local typename ThreadCapture<Type>::ValueType
    ThreadCapture<Type>::current_(nullptr);

}  // namespace capture_thread

#endif  // THREAD_CAPTURE_H_
