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

// This example demonstrates how you might implement asynchronous reporting,
// e.g., in cases where the reporting process is too costly to be done
// synchronously. The callers of the reporting API aren't blocked by the sending
// of the report, which is handled separately by a dedicated reporting thread.

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#include "thread-capture.h"

using capture_thread::ThreadCapture;

// Captures reports for external reporting. Something like this might be used to
// report usage, access, or performance information to an external resource,
// e.g., remote storage or an auditing server.
class Reporter : public ThreadCapture<Reporter> {
 public:
  // Arbitrary content to report, which would generally be more structured.
  using Report = std::list<std::string>;

  // Sends the report, if a reporter is in scope; otherwise does nothing. (Move
  // semantics are assumed here, to cut down on copy operations in both cases.)
  static void Send(Report report) {
    if (GetCurrent()) {
      GetCurrent()->SendReport(std::move(report));
    }
  }

 protected:
  Reporter() = default;
  virtual ~Reporter() = default;

  // Actually performs the reporting.
  virtual void SendReport(Report report) = 0;
};

// Captures reports, but writes them asynchronously to avoid blocking.
class ReportAsync : public Reporter {
 public:
  ReportAsync() : cross_and_capture_to_(this) {}

  ~ReportAsync() {
    {
      std::lock_guard<std::mutex> lock(queue_lock_);
      terminated_ = true;
      queue_wait_.notify_all();
    }
    if (reporter_thread_) {
      std::cerr << "Waiting for reporter thread to finish..." << std::endl;
      reporter_thread_->join();
      std::cerr << "Reporter thread finished." << std::endl;
    }
  }

 protected:
  void SendReport(Report report) override {
    std::lock_guard<std::mutex> lock(queue_lock_);
    if (!terminated_) {
      if (!reporter_thread_) {
        StartThread();
      }
      queue_.push(std::move(report));
      queue_wait_.notify_all();
    }
  }

 private:
  // The thread is started lazily.
  void StartThread() {
    reporter_thread_.reset(
        new std::thread(std::bind(&ReportAsync::ReporterThread, this)));
  }

  // Monitors the queue, and bulk-writes new entries whenever possible.
  void ReporterThread() {
    bool terminated = false;
    std::queue<Report> working_queue;
    while (!terminated) {
      std::unique_lock<std::mutex> lock(queue_lock_);
      // Wait for new entries to report.
      while (!(terminated = terminated_) && queue_.empty()) {
        queue_wait_.wait(lock);
      }
      assert(working_queue.empty());
      // Grab them all at once so that the lock can be released, thereby
      // minimizing the amount of time callers are blocked.
      working_queue.swap(queue_);
      lock.unlock();
      // Process the entries.
      while (!working_queue.empty()) {
        WriteToStorage(working_queue.front());
        working_queue.pop();
      }
    }
  }

  void WriteToStorage(const Report& report) {
    // Simulates an expensive write operation.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (const auto& entry : report) {
      std::cout << entry << std::endl;
    }
  }

  bool terminated_ = false;
  std::mutex queue_lock_;
  std::condition_variable queue_wait_;
  // It's assumed that the average time between calls to Reporter::Send will not
  // exceed the rate at which reports can be written. That isn't a safe
  // assumption in practice, so you would normally want to limit the capacity
  // here and reject reports if that's exceeded.
  std::queue<Report> queue_;
  std::unique_ptr<std::thread> reporter_thread_;
  const AutoThreadCrosser cross_and_capture_to_;
};

// Simulates a service that process external requests.
class DataService {
 public:
  DataService() { std::cerr << "Starting DataService." << std::endl; }

  ~DataService() { std::cerr << "Stopping DataService." << std::endl; }

  // An arbitrary latency-sensitive operation that should not be blocked by the
  // reporting process.
  void AccessSomeResources(int resource_number) {
    Reporter::Report report;
    std::ostringstream formatted;
    formatted << "resource accessed: " << resource_number;
    report.emplace_back(formatted.str());
    Reporter::Send(std::move(report));
  }
};

int main() {
  // Enable reporting globally.
  ReportAsync reporter;

  DataService service;
  for (int i = 0; i < 10; ++i) {
    // Simulates a latency-sensitive request to the service.
    service.AccessSomeResources(i);
  }
}
