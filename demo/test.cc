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

#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "callback-queue.h"
#include "logging.h"
#include "tracing.h"

using capture_thread::ThreadCrosser;
using common::CallbackQueue;
using testing::ElementsAre;

namespace demo {

TEST(DemoTest, IntegrationTest) {
  CaptureLogging logger;
  Tracing context("test");
  CallbackQueue queue;

  for (int i = 0; i < 3; ++i) {
    queue.Push(ThreadCrosser::WrapCall([i] {
      Tracing context("thread");
      Logging::LogLine() << "call " << i;
    }));
  }

  std::thread worker(ThreadCrosser::WrapCall([&queue] {
    Tracing context("worker");
    Logging::LogLine() << "start";
    while (queue.PopAndCall()) {
    }
    Logging::LogLine() << "stop";
  }));

  queue.WaitUntilEmpty();
  queue.Terminate();

  worker.join();

  EXPECT_THAT(logger.CopyLines(),
              ElementsAre("test:worker: start\n", "test:thread: call 0\n",
                          "test:thread: call 1\n", "test:thread: call 2\n",
                          "test:worker: stop\n"));
}

}  // namespace demo

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
