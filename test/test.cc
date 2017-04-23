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

#include "callback-queue.h"
#include "log-text.h"
#include "log-values.h"

using common::CallbackQueue;
using testing::ElementsAre;

namespace capture_thread {

TEST(ThreadCaptureTest, NoLoggerInterferenceWithDifferentTypes) {
  LogText::Log("not logged");
  LogValues::Count(0);
  {
    LogTextSingleThread text_logger;
    LogText::Log("logged 1");
    {
      LogValuesSingleThread count_logger;
      LogValues::Count(1);
      LogText::Log("logged 2");
      EXPECT_THAT(count_logger.GetCounts(), ElementsAre(1));
    }
    LogText::Log("logged 3");
    EXPECT_THAT(text_logger.GetLines(),
                ElementsAre("logged 1", "logged 2", "logged 3"));
  }
}

TEST(ThreadCaptureTest, SameTypeOverrides) {
  LogTextSingleThread text_logger1;
  LogText::Log("logged 1");
  {
    LogTextSingleThread text_logger2;
    LogText::Log("logged 2");
    EXPECT_THAT(text_logger2.GetLines(), ElementsAre("logged 2"));
  }
  LogText::Log("logged 3");
  EXPECT_THAT(text_logger1.GetLines(), ElementsAre("logged 1", "logged 3"));
}

TEST(ThreadCaptureTest, ThreadsAreNotCrossed) {
  LogTextSingleThread logger;
  LogText::Log("logged 1");

  std::thread worker([] { LogText::Log("logged 2"); });
  worker.join();

  EXPECT_THAT(logger.GetLines(), ElementsAre("logged 1"));
}

TEST(ThreadCaptureTest, ManualThreadCrossing) {
  LogTextSingleThread logger;
  LogText::Log("logged 1");

  const LogText::ThreadBridge bridge;
  std::thread worker([&bridge] {
    LogText::CrossThreads logger(bridge);
    LogText::Log("logged 2");
  });
  worker.join();

  EXPECT_THAT(logger.GetLines(), ElementsAre("logged 1", "logged 2"));
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
  LogTextMultiThread logger1;
  bool called = false;
  const auto callback = ThreadCrosser::WrapCall([&called] {
    called = true;
    LogText::Log("logged 1");
  });
  LogTextMultiThread logger2;
  callback();
  EXPECT_TRUE(called);
  EXPECT_THAT(logger1.GetLines(), ElementsAre("logged 1"));
  EXPECT_THAT(logger2.GetLines(), ElementsAre());
}

TEST(ThreadCrosserTest, WrapCallOnlyCapturesCrossers) {
  LogTextMultiThread logger1;
  LogTextSingleThread logger2;
  bool called = false;
  const auto callback = ThreadCrosser::WrapCall([&called] {
    called = true;
    LogText::Log("logged 1");
  });
  callback();
  EXPECT_TRUE(called);
  LogText::Log("logged 2");
  EXPECT_THAT(logger1.GetLines(), ElementsAre("logged 1"));
  EXPECT_THAT(logger2.GetLines(), ElementsAre("logged 2"));
}

TEST(ThreadCrosserTest, WrapCallIsIdempotent) {
  LogTextMultiThread logger1;
  bool called = false;
  const auto callback =
      ThreadCrosser::WrapCall(ThreadCrosser::WrapCall([&called] {
        called = true;
        LogText::Log("logged 1");
      }));
  LogTextMultiThread logger2;
  callback();
  EXPECT_TRUE(called);
  EXPECT_THAT(logger1.GetLines(), ElementsAre("logged 1"));
  EXPECT_THAT(logger2.GetLines(), ElementsAre());
}

TEST(ThreadCrosserTest, WrapCallFallsThroughWithoutLogger) {
  bool called = false;
  const auto callback = ThreadCrosser::WrapCall([&called] {
    called = true;
    LogText::Log("logged 1");
  });
  LogTextMultiThread logger;
  callback();
  EXPECT_TRUE(called);
  EXPECT_THAT(logger.GetLines(), ElementsAre("logged 1"));
}

TEST(ThreadCrosserTest, WrapCallWithNullCallbackIsNull) {
  EXPECT_FALSE(ThreadCrosser::WrapCall(nullptr));
  LogTextMultiThread logger;
  EXPECT_FALSE(ThreadCrosser::WrapCall(nullptr));
}

TEST(ThreadCrosserTest, SingleThreadCrossing) {
  LogTextMultiThread logger;
  LogText::Log("logged 1");

  std::thread worker(ThreadCrosser::WrapCall([] { LogText::Log("logged 2"); }));
  worker.join();

  EXPECT_THAT(logger.GetLines(), ElementsAre("logged 1", "logged 2"));
}

TEST(ThreadCrosserTest, MultipleThreadCrossingWithMultipleLoggers) {
  LogTextMultiThread text_logger;
  LogText::Log("logged 1");
  LogValuesMultiThread count_logger;
  LogValues::Count(1);

  std::thread worker(ThreadCrosser::WrapCall([] {
    std::thread worker(ThreadCrosser::WrapCall([] {
      LogText::Log("logged 2");
      LogValues::Count(2);
    }));
    worker.join();
  }));
  worker.join();

  EXPECT_THAT(text_logger.GetLines(), ElementsAre("logged 1", "logged 2"));
  EXPECT_THAT(count_logger.GetCounts(), ElementsAre(1, 2));
}

TEST(ThreadCrosserTest, MultipleThreadCrossingWithDifferentLoggerScopes) {
  LogTextMultiThread text_logger;

  std::thread worker(ThreadCrosser::WrapCall([] {
    LogValuesMultiThread count_logger;
    std::thread worker(ThreadCrosser::WrapCall([] {
      LogText::Log("logged 1");
      LogValues::Count(1);
    }));
    worker.join();
    EXPECT_THAT(count_logger.GetCounts(), ElementsAre(1));
  }));
  worker.join();

  EXPECT_THAT(text_logger.GetLines(), ElementsAre("logged 1"));
}

TEST(ThreadCrosserTest, MultipleThreadCrossingWithOverride) {
  LogTextMultiThread logger1;

  std::thread worker(ThreadCrosser::WrapCall([] {
    LogTextMultiThread logger2;
    std::thread worker(
        ThreadCrosser::WrapCall([] { LogText::Log("logged 2"); }));
    worker.join();
    EXPECT_THAT(logger2.GetLines(), ElementsAre("logged 2"));
  }));
  worker.join();

  EXPECT_THAT(logger1.GetLines(), ElementsAre());
}

TEST(ThreadCrosserTest, DifferentLoggersInSameThread) {
  CallbackQueue queue;

  std::thread worker([&queue] {
    while (true) {
      LogTextMultiThread logger;
      if (!queue.PopAndCall()) {
        break;
      }
      LogText::Log("logged in thread");
      EXPECT_THAT(logger.GetLines(), ElementsAre("logged in thread"));
    }
  });

  LogTextMultiThread logger1;
  queue.Push(ThreadCrosser::WrapCall([] { LogText::Log("logged 1"); }));
  queue.WaitUntilEmpty();
  EXPECT_THAT(logger1.GetLines(), ElementsAre("logged 1"));

  {
    // It's important for the test case that logger2 overrides logger1, i.e.,
    // that they are both in scope at the same time.
    LogTextMultiThread logger2;
    queue.Push(ThreadCrosser::WrapCall([] { LogText::Log("logged 2"); }));
    queue.WaitUntilEmpty();
    EXPECT_THAT(logger2.GetLines(), ElementsAre("logged 2"));
  }

  queue.Push(ThreadCrosser::WrapCall([] { LogText::Log("logged 3"); }));
  queue.WaitUntilEmpty();
  EXPECT_THAT(logger1.GetLines(), ElementsAre("logged 1", "logged 3"));

  queue.Terminate();
  worker.join();
}

TEST(ThreadCrosserTest, ReverseOrderOfLoggersOnStack) {
  LogTextMultiThread logger1;
  const auto callback =
      ThreadCrosser::WrapCall([] { LogText::Log("logged 1"); });

  LogTextMultiThread logger2;
  const auto worker_call = ThreadCrosser::WrapCall([callback] {
    // In callback(), logger1 overrides logger2, whereas in the main thread
    // logger2 overrides logger1.
    callback();
    LogText::Log("logged 2");
  });

  LogTextMultiThread logger3;

  // Call using a thread.
  std::thread worker(worker_call);
  worker.join();

  EXPECT_THAT(logger1.GetLines(), ElementsAre("logged 1"));
  EXPECT_THAT(logger2.GetLines(), ElementsAre("logged 2"));
  EXPECT_THAT(logger3.GetLines(), ElementsAre());

  // Call in the main thread.
  worker_call();

  EXPECT_THAT(logger1.GetLines(), ElementsAre("logged 1", "logged 1"));
  EXPECT_THAT(logger2.GetLines(), ElementsAre("logged 2", "logged 2"));
  EXPECT_THAT(logger3.GetLines(), ElementsAre());
}

TEST(ThreadCrosserTest, ManualCrosserOverride) {
  LogTextMultiThread logger;
  const ThreadCrosser::SetOverride override_point;

  std::thread unwrapped_worker([&override_point] {
    override_point.Call([] {
      LogText::Log("logged 1");
    });
  });
  unwrapped_worker.join();

  EXPECT_THAT(logger.GetLines(), ElementsAre("logged 1"));
}

TEST(ThreadCrosserTest, ManualOverrideIndependentOfNormalScope) {
  LogTextMultiThread text_logger1;
  const ThreadCrosser::SetOverride override_point;

  std::thread unwrapped_worker([&override_point] {
    LogTextMultiThread text_logger2;
    LogValuesMultiThread count_logger;
    // Global override of LogText but not LogValues.
    override_point.Call([] {
      LogText::Log("logged 1");
      LogValues::Count(1);
    });
    // Local scope supercedes global override of LogText.
    override_point.Call(ThreadCrosser::WrapCall([] {
      LogText::Log("logged 2");
      LogValues::Count(2);
    }));
    EXPECT_THAT(text_logger2.GetLines(), ElementsAre("logged 2"));
    EXPECT_THAT(count_logger.GetCounts(), ElementsAre(1, 2));
  });
  unwrapped_worker.join();

  EXPECT_THAT(text_logger1.GetLines(), ElementsAre("logged 1"));
}

}  // namespace capture_thread

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
