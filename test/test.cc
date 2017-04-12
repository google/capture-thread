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

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "thread-capture.h"
#include "thread-crosser.h"

using testing::ElementsAre;

namespace capture_thread {

class LogText : public ThreadCapture<LogText> {
 public:
  LogText() : cross_and_capture_to_(this) {}

  static void Log(std::string line) {
    if (GetCurrent()) {
      GetCurrent()->LogLine(std::move(line));
    }
  }

  const std::list<std::string> GetLinesUnsafe() { return lines_; }

 private:
  void LogLine(std::string line) {
    std::lock_guard<std::mutex> lock(data_lock_);
    lines_.emplace_back(std::move(line));
  }

  std::mutex data_lock_;
  std::list<std::string> lines_;
  const AutoThreadCrosser cross_and_capture_to_;
};

class LogValues : public ThreadCapture<LogValues> {
 public:
  LogValues() : cross_and_capture_to_(this) {}

  static void Count(int count) {
    if (GetCurrent()) {
      GetCurrent()->LogCount(count);
    }
  }

  const std::list<int> GetCountsUnsafe() { return counts_; }

 private:
  void LogCount(int count) {
    std::lock_guard<std::mutex> lock(data_lock_);
    counts_.emplace_back(count);
  }

  std::mutex data_lock_;
  std::list<int> counts_;
  const AutoThreadCrosser cross_and_capture_to_;
};

class BlockingCallbackQueue {
 public:
  void Push(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(queue_lock_);
    queue_.push(std::move(callback));
    condition_.notify_all();
  }

  // NOTE: Calling before returning avoids a race condition if WaitUntilEmpty()
  // is used to wait until all calls complete.
  bool PopAndCall() {
    std::unique_lock<std::mutex> lock(queue_lock_);
    while (!terminated_ && queue_.empty()) {
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

  void WaitUntilEmpty() {
    std::unique_lock<std::mutex> lock(queue_lock_);
    while (!terminated_ && (!queue_.empty() || pending_ > 0)) {
      condition_.wait(lock);
    }
  }

  void Terminate() {
    std::lock_guard<std::mutex> lock(queue_lock_);
    terminated_ = true;
    condition_.notify_all();
  }

 private:
  std::mutex queue_lock_;
  std::condition_variable condition_;
  int pending_ = 0;
  bool terminated_ = false;
  std::queue<std::function<void()>> queue_;
};

TEST(ThreadCaptureTest, NoLoggerInterferenceWithDifferentTypes) {
  LogText::Log("not logged");
  LogValues::Count(0);
  {
    LogText text_logger;
    LogText::Log("logged 1");
    {
      LogValues count_logger;
      LogValues::Count(1);
      LogText::Log("logged 2");
      EXPECT_THAT(count_logger.GetCountsUnsafe(), ElementsAre(1));
    }
    LogText::Log("logged 3");
    EXPECT_THAT(text_logger.GetLinesUnsafe(),
                ElementsAre("logged 1", "logged 2", "logged 3"));
  }
}

TEST(ThreadCaptureTest, SameTypeOverrides) {
  LogText text_logger1;
  LogText::Log("logged 1");
  {
    LogText text_logger2;
    LogText::Log("logged 2");
    EXPECT_THAT(text_logger2.GetLinesUnsafe(), ElementsAre("logged 2"));
  }
  LogText::Log("logged 3");
  EXPECT_THAT(text_logger1.GetLinesUnsafe(),
              ElementsAre("logged 1", "logged 3"));
}

TEST(ThreadCaptureTest, ThreadsAreNotCrossed) {
  LogText logger;
  LogText::Log("logged 1");

  std::thread worker([] { LogText::Log("logged 2"); });
  worker.join();

  EXPECT_THAT(logger.GetLinesUnsafe(), ElementsAre("logged 1"));
}

TEST(ThreadCaptureTest, ManualThreadCrossing) {
  LogText logger;
  LogText::Log("logged 1");

  const LogText::ThreadBridge bridge;
  std::thread worker([&bridge] {
    LogText::CrossThreads logger(bridge);
    LogText::Log("logged 2");
  });
  worker.join();

  EXPECT_THAT(logger.GetLinesUnsafe(), ElementsAre("logged 1", "logged 2"));
}

TEST(ThreadCrosserTest, WrapCallIsFineWithoutLogger) {
  bool called = false;
  ThreadCrosser::WrapCall([&called] {
    called = true;
    LogText::Log("not logged");
  })();
  EXPECT_TRUE(called);
}

TEST(ThreadCrosserTest, WrapCallIsNotLazy) {
  LogText logger1;
  bool called = false;
  const auto callback = ThreadCrosser::WrapCall([&called] {
    called = true;
    LogText::Log("logged 1");
  });
  LogText logger2;
  callback();
  EXPECT_TRUE(called);
  EXPECT_THAT(logger1.GetLinesUnsafe(), ElementsAre("logged 1"));
  EXPECT_THAT(logger2.GetLinesUnsafe(), ElementsAre());
}

TEST(ThreadCrosserTest, WrapCallIsIdempotent) {
  LogText logger1;
  bool called = false;
  const auto callback =
      ThreadCrosser::WrapCall(ThreadCrosser::WrapCall([&called] {
        called = true;
        LogText::Log("logged 1");
      }));
  LogText logger2;
  callback();
  EXPECT_TRUE(called);
  EXPECT_THAT(logger1.GetLinesUnsafe(), ElementsAre("logged 1"));
  EXPECT_THAT(logger2.GetLinesUnsafe(), ElementsAre());
}

TEST(ThreadCrosserTest, WrapCallFallsThroughWithoutLogger) {
  bool called = false;
  const auto callback = ThreadCrosser::WrapCall([&called] {
    called = true;
    LogText::Log("logged 1");
  });
  LogText logger;
  callback();
  EXPECT_TRUE(called);
  EXPECT_THAT(logger.GetLinesUnsafe(), ElementsAre("logged 1"));
}

TEST(ThreadCrosserTest, WrapCallWithNullCallbackIsNull) {
  EXPECT_FALSE(ThreadCrosser::WrapCall(nullptr));
  LogText logger;
  EXPECT_FALSE(ThreadCrosser::WrapCall(nullptr));
}

TEST(ThreadCrosserTest, SingleThreadCrossing) {
  LogText logger;
  LogText::Log("logged 1");

  std::thread worker(ThreadCrosser::WrapCall([] { LogText::Log("logged 2"); }));
  worker.join();

  EXPECT_THAT(logger.GetLinesUnsafe(), ElementsAre("logged 1", "logged 2"));
}

TEST(ThreadCrosserTest, MultipleThreadCrossingWithMultipleLoggers) {
  LogText text_logger;
  LogText::Log("logged 1");
  LogValues count_logger;
  LogValues::Count(1);

  std::thread worker(ThreadCrosser::WrapCall([] {
    std::thread worker(ThreadCrosser::WrapCall([] {
      LogText::Log("logged 2");
      LogValues::Count(2);
    }));
    worker.join();
  }));
  worker.join();

  EXPECT_THAT(text_logger.GetLinesUnsafe(),
              ElementsAre("logged 1", "logged 2"));
  EXPECT_THAT(count_logger.GetCountsUnsafe(), ElementsAre(1, 2));
}

TEST(ThreadCrosserTest, MultipleThreadCrossingWithDifferentLoggerScopes) {
  LogText text_logger;

  std::thread worker(ThreadCrosser::WrapCall([] {
    LogValues count_logger;
    std::thread worker(ThreadCrosser::WrapCall([] {
      LogText::Log("logged 1");
      LogValues::Count(1);
    }));
    worker.join();
    EXPECT_THAT(count_logger.GetCountsUnsafe(), ElementsAre(1));
  }));
  worker.join();

  EXPECT_THAT(text_logger.GetLinesUnsafe(), ElementsAre("logged 1"));
}

TEST(ThreadCrosserTest, MultipleThreadCrossingWithOverride) {
  LogText logger1;

  std::thread worker(ThreadCrosser::WrapCall([] {
    LogText logger2;
    std::thread worker(
        ThreadCrosser::WrapCall([] { LogText::Log("logged 2"); }));
    worker.join();
    EXPECT_THAT(logger2.GetLinesUnsafe(), ElementsAre("logged 2"));
  }));
  worker.join();

  EXPECT_THAT(logger1.GetLinesUnsafe(), ElementsAre());
}

TEST(ThreadCrosserTest, DifferentLoggersInSameThread) {
  BlockingCallbackQueue queue;

  std::thread worker([&queue] {
    while (true) {
      LogText logger;
      if (!queue.PopAndCall()) {
        break;
      }
      LogText::Log("logged in thread");
      EXPECT_THAT(logger.GetLinesUnsafe(), ElementsAre("logged in thread"));
    }
  });

  LogText logger1;
  queue.Push(ThreadCrosser::WrapCall([] { LogText::Log("logged 1"); }));
  queue.WaitUntilEmpty();
  EXPECT_THAT(logger1.GetLinesUnsafe(), ElementsAre("logged 1"));

  {
    // It's important for the test case that logger2 overrides logger1, i.e.,
    // that they are both in scope at the same time.
    LogText logger2;
    queue.Push(ThreadCrosser::WrapCall([] { LogText::Log("logged 2"); }));
    queue.WaitUntilEmpty();
    EXPECT_THAT(logger2.GetLinesUnsafe(), ElementsAre("logged 2"));
  }

  queue.Push(ThreadCrosser::WrapCall([] { LogText::Log("logged 3"); }));
  queue.WaitUntilEmpty();
  EXPECT_THAT(logger1.GetLinesUnsafe(), ElementsAre("logged 1", "logged 3"));

  queue.Terminate();
  worker.join();
}

TEST(ThreadCrosserTest, ReverseOrderOfLoggersOnStack) {
  LogText logger1;
  const auto callback =
      ThreadCrosser::WrapCall([] { LogText::Log("logged 1"); });

  LogText logger2;
  const auto worker_call = ThreadCrosser::WrapCall([callback] {
    // In callback(), logger1 overrides logger2, whereas in the main thread
    // logger2 overrides logger1.
    callback();
    LogText::Log("logged 2");
  });

  LogText logger3;

  // Call using a thread.
  std::thread worker(worker_call);
  worker.join();

  EXPECT_THAT(logger1.GetLinesUnsafe(), ElementsAre("logged 1"));
  EXPECT_THAT(logger2.GetLinesUnsafe(), ElementsAre("logged 2"));
  EXPECT_THAT(logger3.GetLinesUnsafe(), ElementsAre());

  // Call in the main thread.
  worker_call();

  EXPECT_THAT(logger1.GetLinesUnsafe(), ElementsAre("logged 1", "logged 1"));
  EXPECT_THAT(logger2.GetLinesUnsafe(), ElementsAre("logged 2", "logged 2"));
  EXPECT_THAT(logger3.GetLinesUnsafe(), ElementsAre());
}

}  // namespace capture_thread

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
