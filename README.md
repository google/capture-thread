# [Capture Thread Library][google/capture-thread]

Framework for loggers, tracers, and mockers in multithreaded C++ programs.

*(This is not an official Google product.)*

## Motivation

This library solves the following problem: *Information needs to be passed
between different levels of implementation, but that information isn't an
integral part of the system's design.*

For example:

1.  Loggers that collect information about the inner workings of the system,
    e.g., performance and call information.
2.  Tracers that provide the inner workings of the system with context regarding
    the execution path taken to get to a certain call.
3.  Mockers that need to replace a real object with a mock object without being
    able to do so via dependency injection.

Instrumenting C++ code can be difficult and ugly, and once finished it can also
cause the original code to be difficult to maintain:

-   Explicitly passing objects down from the top causes code to become
    unmaintainable.
-   Using global data can cause serious complications in multithreaded
    environments when you only want to log, trace, or mock a certain call and
    not others.
-   Using a unique identifier to look up shared data in a global hash table
    requires both creating an identifier and passing it around.

This library circumvents these problems by tracking information per thread, with
the possibility of conditionally sharing information between threads.

**Most importantly, this is a framework for your loggers, tracers, and mockers,
and not for your entire project. This means that by design it's non-intrusive,
and doesn't require you to modify the design of your project.**

## Quick Start

Instrumenting a project has four steps. These assume that your project is
already functional, and is just lacking instrumentation.

1.  Create an instrumentation class to contain the state to be shared.
2.  Instrument your project with logging points, tracing points, or mocking
    substitutions, depending on which you implemented.
3.  Where control is passed between threads, e.g., creating a thread or passing
    a callback between threads, use the logic in `ThreadCrosser` to ensure that
    the instrumentation crosses threads.
4.  As needed, instantiate the instrumentation class(es) to enable the
    instrumentation within a specific scope.

Complexity estimates below are estimates of how much additional work will be
necessary as your project grows.

### Step 1: Instrumentation Class [*O(1)*]

The instrumentation class(es) will generally be written once and then left
alone. They might also be general enough for use in multiple projects.

```c++
#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include "thread-capture.h"

// This class provides the instrumentation logic, both at the point the
// instrumentation is used (e.g., logging points) and where it is enabled (e.g.,
// log-capture points.) Note that instances of ThreadCapture cannot be moved,
// copied, or dynamically allocated.
class Logger : capture_thread::ThreadCapture<Logger> {
 public:
  Logger() : cross_and_capture_to_(this) {}

  // The static API is used at the instrumentation points. It will often be a
  // no-op if no instrumentation is in scope.
  static void Log(const std::string& line) {
    // GetCurrent() provides the instrumentation currently in scope, and is
    // always thread-safe and repeatable. The implementation of the
    // instrumentation must be explicitly made thread-safe, however.
    if (GetCurrent()) {
      GetCurrent()->LogLine(line);
    } else {
      std::cerr << "Not captured: \"" << line << "\"" << std::endl;
    }
  }

  // The non-static public API allows the creator of the instrumentation object
  // to access its contents. This is only necessary when the instrumentation is
  // gathering information, as opposed to propagating information.
  std::list<std::string> GetLines() {
    std::lock_guard<std::mutex> lock(lock_);
    return lines_;
  }

 private:
  // The private implementation applies to the instrumentation only when it's in
  // scope. This does not need to exactly mirror the static API, and in fact
  // only needs to differentiate between default and override behaviors.
  void LogLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(lock_);
    lines_.emplace_back(line);
  }

  std::mutex lock_;
  std::list<std::string> lines_;
  // Add an AutoThreadCrosser to ensure that scoping is handled correctly. If
  // you absolutely don't want the instrumentation crossing threads, use
  // ScopedCapture instead. Always initialize with `this`.
  const AutoThreadCrosser cross_and_capture_to_;
};
```

### Step 2: Instrument the Code [*O(n)*]

Instrumenting the code with your new instrumentation class will generally
consist of one-line additions throughout the code. There will often be a large
number of instrumentation points in the code.

```c++
// #include the header for your instrumentation class.

// This function already exists in your code, and performs some sort of work for
// which you want to use the instrumentation.
void MyExistingFunction() {
  // Add calls to the static API where you need access to the instrumentation.
  Logger::Log("MyExistingFunction called");
}
```

### Step 3: Cross Threads [*O(log n)*]

Crossing threads is necessary when the process you are tracking splits work
among multiple threads. The complexity here depends on both what you consider a
single task (e.g., processing a query) and how that work is split among threads.

```c++
#include <thread>
#include "thread-crosser.h"

// (You don't need to #include the header for your instrumentation class here.)

// This function already exists in your code, and parallelizes some
// functionality that needs to use the instrumentation, but doesn't need to use
// the instrumentation itself.
void ParallelizeWork() {
  // Previously, the code just created a thread.
  // std::thread worker(&MyExistingFunction);

  // To pass along the instrumentation, wrap the thread with WrapCall. Also use
  // WrapCall when passing work to a worker thread, e.g., a thread pool.
  std::thread worker(
      capture_thread::ThreadCrosser::WrapCall(&MyExistingFunction));
  worker.join();
}
```

### Step 4: Enable Instrumentation [*O(1)*]

Your instrumentation must have default behavior that makes sense when the
instrumentation isn't enabled. Instrumentation can only be enabled by
instantiating the implementation, and will only be available until that instance
goes out of scope. There should be *very few* instantiation points (i.e.,
usually just one per instrumentation type) in your code.

```c++
// #include the header for your instrumentation class.

int main() {
  // If no instrumentation is in scope, the default behavior of the static API
  // is used where instrumentation calls are made. In this case, this will just
  // print the line to std::cerr.
  ParallelizeWork();

  // To make the instrumentation available within a given scope, just
  // instantiate your class. The framework will take care of the rest.
  Logger logger;

  // Since a Logger is in scope, the line will be captured to that instance,
  // rather than the default behavior of printing to std::cerr.
  ParallelizeWork();

  // In this case the instrumentation captures data, which is now available in
  // the local instance.
  for (const std::string& line : logger.GetLines()) {
    std::cerr << "The logger captured: \"" << line << "\"" << std::endl;
  }
}
```

The instantiation point will depend on the semantics you are going for. For
example, if you are mocking, you might only instantiate the instrumentation in
unit tests, and use the default behavior in the released code.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines. All contributions must
follow the Google C++ style guide at
https://google.github.io/styleguide/cppguide.html. **Contributions should be
made to the [`current`][current] branch, which will periodically be merged with
[`master`][master] after a more thorough review.**

[google/capture-thread]: https://github.com/google/capture-thread
[master]: https://github.com/google/capture-thread/tree/master
[current]: https://github.com/google/capture-thread/tree/current
