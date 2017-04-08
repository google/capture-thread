#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "thread-capture.h"
#include "thread-crosser.h"

// See simple.cc for comments. The only comments here relate to threading.)
class LogText : public ThreadCapture<LogText> {
 public:
  LogText() : cross_and_capture_to_(this) {}

  static void Log(std::string line) {
    if (GetCurrent()) {
      GetCurrent()->LogLine(std::move(line));
    }
  }

  std::list<std::string> CopyLines() {
    std::lock_guard<std::mutex> lock(data_lock_);
    return lines_;
  }

 private:
  void LogLine(std::string line) {
    std::lock_guard<std::mutex> lock(data_lock_);
    lines_.emplace_back(std::move(line));
  }

  // To support threading, just add mutex protection and an AutoThreadCrosser.
  // AutoThreadCrosser ensures that logging is passed on to worker threads, but
  // *only* when the thread function is wrapped with ThreadCrosser::WrapCall.

  // Note that if you implement multiple types of ThreadCapture in this way, a
  // single call to ThreadCrosser::WrapCall will pass on *all* of them that are
  // currently in scope in the current thread. For example, if you're logging
  // strings (e.g., this class) and have a separate ThreadCapture to log timing
  // that also uses AutoThreadCrosser, a single call to ThreadCrosser::WrapCall
  // will pass on both string logging and timing logging.

  std::mutex data_lock_;
  std::list<std::string> lines_;
  const AutoThreadCrosser<LogText> cross_and_capture_to_;
};

int main() {
  LogText::Log("No logger is in scope.");
  {
    LogText logger;
    LogText::Log("The logger is in scope.");

    std::thread logged_thread(ThreadCrosser::WrapCall([] {
          LogText::Log("ThreadCrosser::WrapCall passes on logging.");
        }));

    std::thread unlogged_thread([] {
          LogText::Log("Logging has not been passed on here.");
        });

    logged_thread.join();
    unlogged_thread.join();

    for (const auto& line : logger.CopyLines()) {
      std::cerr << "Captured: " << line << std::endl;
    }
  }
}
