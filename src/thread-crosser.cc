#include "thread-crosser.h"

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
