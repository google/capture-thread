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

}  // namespace capture_thread

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
