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

#include <cassert>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "thread-capture.h"

using capture_thread::ThreadCapture;
using capture_thread::ThreadCrosser;

// A simple logger to capture usage of the server we are going to implement.
// (See threaded.cc for more background.)
class LogUsage : public ThreadCapture<LogUsage> {
 public:
  LogUsage() : cross_and_capture_to_(this) {}

  static void Query(std::string query) {
    if (GetCurrent()) {
      GetCurrent()->LogQuery(std::move(query));
    }
  }

  std::list<std::string> CopyQueries() {
    std::lock_guard<std::mutex> lock(data_lock_);
    return queries_;
  }

 private:
  void LogQuery(std::string query) {
    std::lock_guard<std::mutex> lock(data_lock_);
    queries_.emplace_back(std::move(query));
  }

  std::mutex data_lock_;
  std::list<std::string> queries_;
  // AutoThreadCrosser ensures that ThreadCrosser captures this logger.
  const AutoThreadCrosser cross_and_capture_to_;
};

// Some third-party framework that is used by deriving a handler class. (Note
// that this class represents a framework that you don't have control over, and
// is not here as something to be replicated.)
class ThirdPartyFramework {
 public:
  // This is the API the framework provides to its users, and is the only
  // control over execution that the framework provides.
  class ServerInterface {
   public:
    virtual ~ServerInterface() = default;
    virtual void HandleQuery(const std::string& query) = 0;
  };

  // Our implementation is passed to the constructor, which is then used by the
  // framework to handle calls.
  explicit ThirdPartyFramework(std::unique_ptr<ServerInterface> interface)
      : interface_(std::move(interface)) {}

  // (We pass a list of queries here only to simulate the framework receiving
  // them from somewhere else, e.g., an HTTP client.)
  void Run(const std::list<std::string>& faked_queries) const {
    assert(interface_);
    // We have no idea if the framework will execute our implementation in the
    // same thread that the ServerInterface was constructed in. (This is
    // actually the entire motivation for this example.)
    std::thread worker_thread([&] {
      for (const auto query : faked_queries) {
        interface_->HandleQuery(query);
      }
    });
    worker_thread.join();
  }

 private:
  const std::unique_ptr<ServerInterface> interface_;
};

// Utilizing the third-party framework is done by implementing the limited
// interface the framework provides. Instances of this class should never
// outlive the scope that they are created in; see the override_point below.
class MyServer : public ThirdPartyFramework::ServerInterface {
 public:
  void HandleQuery(const std::string& query) override {
    // SetOverride::Call has most of the semantics of ThreadCrosser::WrapCall,
    // but with different calling conventions:
    // 1. SetOverride::Call is a member function.
    // 2. SetOverride::Call actually calls the function that is passed to it.
    // 3. SetOverride::Call is called from the worker thread, whereas
    //    ThreadCrosser::WrapCall is called from the main thread.
    override_point.Call([&] {
      // Within this lambda, the scope override-point set by override_point is
      // temporarily used. Call doesn't need to wrap the entire implementation;
      // presumably you'll know exactly what functionality needs access to the
      // scope that is being passed along. (Again, SetOverride should really
      // only be used when ThreadCrosser::WrapCall can't be.)
      std::cerr << "MyServer is processing query \""
                << query << "\"" << std::endl;
      LogUsage::Query(query);
    });
    // Outside of the Call, override_point has no effect.
  }

 private:
  // ThreadCrosser::SetOverride will capture the current scope when the
  // constructor is called. This makes that scope available to this class when
  // the framework makes calls to the API in different threads.
  const ThreadCrosser::SetOverride override_point;
};

int main() {
  // You are interested in some sort of global logging for the process.
  LogUsage query_log;

  // Since MyServer contains ThreadCrosser::SetOverride, it will capture the
  // current scope. When the framework calls the API, the behavior should be the
  // same whether or not those calls are in this thread or in another thread.
  std::unique_ptr<ThirdPartyFramework::ServerInterface> interface(new MyServer);

  const ThirdPartyFramework server(std::move(interface));
  server.Run({"query 1", "query 2", "query 3"});

  for (const auto& query : query_log.CopyQueries()) {
    std::cerr << "Captured query: \"" << query << "\"" << std::endl;
  }
}
