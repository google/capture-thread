#ifndef THREAD_CAPTURE_H_
#define THREAD_CAPTURE_H_

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

#endif  // THREAD_CAPTURE_H_
