#ifndef THIRD_PARTY_CAPTURE_THREAD_THREAD_LOCAL_H_
#define THIRD_PARTY_CAPTURE_THREAD_THREAD_LOCAL_H_

// Use thread_local by default
#if !defined(THREAD_CAPTURE_USE_PTHREAD) && \
    !defined(THREAD_CAPTURE_USE_THREAD_LOCAL)
#define THREAD_CAPTURE_USE_THREAD_LOCAL
#endif

#if defined(THREAD_CAPTURE_USE_PTHREAD)
#include <pthread.h>
#endif

namespace capture_thread {

// Private class for implementing static thread local storage. It allows falling
// back to using pthreads on platforms that don't support the thread_local
// keyword.
template <typename Type>
class ThreadLocal {
 public:
  static Type* GetCurrentThreadValue();
  static void SetCurrentThreadValue(Type* value);

 private:
  ThreadLocal();
  ~ThreadLocal();
  ThreadLocal(const ThreadLocal&) = delete;
  ThreadLocal& operator=(const ThreadLocal&) = delete;

#if defined(THREAD_CAPTURE_USE_THREAD_LOCAL)
  static thread_local Type* value_;
#elif THREAD_CAPTURE_USE_PTHREAD
  static pthread_key_t GetKey();
  static pthread_key_t key_;
#endif
};

#if defined(THREAD_CAPTURE_USE_THREAD_LOCAL)
template <typename Type>
thread_local Type* ThreadLocal<Type>::value_(nullptr);

template <typename Type>
Type* ThreadLocal<Type>::GetCurrentThreadValue() {
  return value_;
}

template <typename Type>
void ThreadLocal<Type>::SetCurrentThreadValue(Type* value) {
  value_ = value;
}
#elif THREAD_CAPTURE_USE_PTHREAD

template <typename Type>
pthread_key_t ThreadLocal<Type>::GetKey() {
  // Always get the key through this function so pthread_key_create can be
  // delayed until the first set/get.
  static int key_create_result __attribute__((__unused__)) =
      pthread_key_create(&key_, [](void*) { pthread_key_delete(key_); });
  return key_;
}

template <typename Type>
Type* ThreadLocal<Type>::GetCurrentThreadValue() {
  return static_cast<Type*>(pthread_getspecific(GetKey()));
}

template <typename Type>
void ThreadLocal<Type>::SetCurrentThreadValue(Type* value) {
  pthread_setspecific(GetKey(), value);
}
#endif

}  // namespace capture_thread

#endif  // THIRD_PARTY_CAPTURE_THREAD_THREAD_LOCAL_H_
