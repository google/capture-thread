# [Capture Thread Library][google/capture-thread]

Framework for loggers, tracers, and mockers in multithreaded C++ programs.

*(This is not an official Google product.)*

## Motivation

When developing C++ projects, [instrumentation][instrumentation] is frequently
used to collect information from the system, inject information into the system,
or both. The role of this information within the system rarely lines up with the
actual structure of the project.

For example:

-   *Loggers* will collect information in a wide range of contexts within the
    code, and thus the logic connecting the log points to the logger will not
    match the structure of the project.

-   *Tracers* make available information that describes how call execution
    arrived at a certain point. In some cases this information can be passed
    along with the data being processed, but in most cases this is obtrusive and
    does not scale well.

-   *Mockers* replace default idioms (e.g., opening files) with alternate
    behavior for the purposes of testing. These actions generally occur deep
    within the code, and could not otherwise be swapped out for testing without
    leaking those details in the API.

This library is designed to handle all of these situations with minimal
intrusion into your project, and without leaking details in your API.

## Summary

The **Capture Thread Library** is designed around the concept of
thread-locality, which allows the sharing of static variables *only within the
current thread*. Canonical static variables, on the other hand, are problematic
due to ownership and thread-safety issues.

This library establishes the following idiom *(using logging as an example)*:

1.  The instrumentation is 100% passive unless it is explicitly enabled. *(For
    example, logging points that by default just print to `std::cerr`.)*

2.  Instrumentation is enabled in the current thread by *instantiating* an
    implementation, and only remains enabled until that instance goes out of
    scope. *(For example, an implementation that captures logged lines while
    it's in scope.)*

3.  While enabled, the instrumentation transparently alters the behavior of
    logic deep within the code that would otherwise use default behavior. *(For
    example, the log-capture instance redirects messages to a `std::list` only
    while it's in scope.)*

4.  Instrumentation can be shared across threads in *explicitly-specified*
    points in the code, based entirely on how you compartmentalize your logic.
    *(For example, independent of the instrumentation, you logically split
    processing of a query into multiple threads, and want the instrumentation to
    treat it as a single process.)*

## Key Design Points

The **Capture Thread Library** has several design points that make it efficient
and reliable:

-   All data structures are immutable and contain no dynamic allocation.
-   All library logic is thread-safe without threads blocking each other.
-   The enabling and disabling of instrumentation is strictly scope-driven,
    making it impossible to have bad pointers when used correctly.
-   The work required to share instrumentation between threads *does not* depend
    on the number of types of instrumentation used. (For example, sharing a
    logger and a tracer is the same amount of work as just sharing a logger.)
-   Instrumentation classes derived (by you) from this library *cannot* be
    moved, copied, or dynamically allocated, ensuring that scoping rules are
    strictly enforced.
-   The library is designed so that instrumentation data structures and calls do
    not need to be visible in your project's API headers, making them low-risk
    to add, modify, or remove.
-   All of the library code is thoroughly unit-tested.

## Caveats

In some cases, it might not be appropriate (or possible) to use the **Capture
Thread Library**:

-   By design, this library is meant to help you *circumvent* the structure of
    your program, specifically so that you don't need to modify your design to
    facilitate instrumentation. Although you could technically structure the
    business logic of your program around this library, doing so would likely
    lead to difficult-to-follow code and baffling latent bugs.

-   If you want to share instrumentation between threads, you *must* be able to
    pass either a `std::function` or a pointer from the source thread to the
    destination thread. This is because those semantics are a part of your
    project's design, and thus cannot be automatically inferred by the library.

-   Since this library is scope-driven, you *cannot* share instrumentation
    outside of the scope it was created in. For example:

    ```c++
    void f() { MyLogger capture_messages; }
    void g() { MyLogger::Log("g was called"); }

    int main() {
      f();
      g();
    }
    ```

    In this example, the instrumentation is created in `f`, but goes out of
    scope before `g` is called. In this case, `g` just uses the default behavior
    for `MyLogger::Log`.

    This is actually dangerous if you return a wrapped function in a way that
    changes the scope:

    ```c++
    // Fine, because no wrapping is done.
    std::function<void()> f() {
      MyLogger capture_messages;
      return [] { MyLogger::Log("f was called"); };
    }

    // Fine, because no instrumentation goes out of scope.
    std::function<void()> g() {
      return ThreadCrosser::WrapCall([] { MyLogger::Log("g was called"); });
    }

    // DANGER! capture_messages goes out of scope, invalidating the function.
    std::function<void()> h() {
      MyLogger capture_messages;
      return ThreadCrosser::WrapCall([] { MyLogger::Log("h was called"); });
    }

    int main() {
      f()();  // Fine.
      g()();  // Fine.
      h()();  // SIGSEGV!
    }
    ```

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

### Step 1: Instrumentation Class [`O(1)`]

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

### Step 2: Instrument the Code [`O(n)`]

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

### Step 3: Cross Threads [`O(log n)`]

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

### Step 4: Enable Instrumentation [`O(1)`]

Your instrumentation must have default behavior that makes sense when the
instrumentation is not enabled. Instrumentation can only be enabled by
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
[instrumentation]: https://en.wikipedia.org/wiki/Instrumentation_(computer_programming)
