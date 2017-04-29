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

## Using

This library contains a very simple framework for creating a logger, tracer, or
mocker that operates in the current thread. The library also contains simple
logic for sharing this context across threads.

The following steps are involved in instrumenting a project:

1.  Write a class to contain the information that will be shared. See
    [`simple.cc`](example/simple.cc) and [`threaded.cc`](example/threaded.cc)
    for the two primary ways of doing this.
2.  Update your implementation code to use the static API of your class to send
    or receive information.
3.  Where appropriate, instantiate your class to make logging, tracing, or
    mocking (whichever you implemented) available in that scope.
4.  If you need to work with multiple threads, e.g., tracing a call from one
    thread to another, wrap thread callbacks in `ThreadCrosser::WrapCall`.

See the [`example`](example) directory for examples of several types of
implementation. [`demo`](demo) contains a larger demo spread across multiple
files, and includes its own unit test. Lastly, unit tests in [`test`](test)
demonstrate a wide range of usage patterns.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines. All contributions must
follow the Google C++ style guide at
https://google.github.io/styleguide/cppguide.html. **Contributions should be
made to the [`current`][current] branch, which will periodically be merged with
[`master`][master] after a more thorough review.**

[google/capture-thread]: https://github.com/google/capture-thread
[master]: https://github.com/google/capture-thread/tree/master
[current]: https://github.com/google/capture-thread/tree/current
