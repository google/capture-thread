#include <iostream>
#include <list>
#include <string>

#include "thread-capture.h"

// Inherit from ThreadCapture, passing the class name as the template argument.
class LogText : public ThreadCapture<LogText> {
 public:
  LogText() : capture_to_(this) {}

  // The public *capturing* API is static, and accesses the private API via
  // GetCurrent().
  static void Log(std::string line) {
    if (GetCurrent()) {
      GetCurrent()->LogLine(std::move(line));
    } else {
      std::cerr << "*** Not captured: \"" << line << "\" ***" << std::endl;
    }
  }

  // The public *accessing* API is non-static, and provides the accumulated
  // information in whatever format happens to be useful.
  const std::list<std::string>& GetLines() const {
    return lines_;
  }

 private:
  // The private API records data when this logger is being captured to.
  void LogLine(std::string line) {
    lines_.emplace_back(std::move(line));
  }

  // Accumulated data.
  std::list<std::string> lines_;

  // ScopedCapture ensures that the static API logs to this object when:
  // 1. This object is in scope.
  // 2. No logger of the *same* type is higher in the current stack.
  const ScopedCapture capture_to_;
};

void NoLogger() {
  LogText::Log("No logger is in scope.");
}

void LoggedOp1() {
  LogText::Log("The logger is in scope.");
}

void LoggedOp2() {
  LogText::Log("It captures all lines.");
}

int main() {
  NoLogger();
  {
    LogText logger;
    LoggedOp1();
    LoggedOp2();

    for (const auto& line : logger.GetLines()) {
      std::cerr << "Captured: " << line << std::endl;
    }
  }
}
