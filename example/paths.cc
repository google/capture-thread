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
#include <sstream>
#include <string>
#include <vector>

#include "thread-capture.h"

using capture_thread::ThreadCapture;

// A helper class for building paths from components.
class PathBuilder {
 public:
  std::string String() const { return path_.str(); }

  PathBuilder& Add(const std::string& component) {
    if (!component.empty()) {
      if (component.front() == '/') {
        path_.str("");
        path_ << component;
      } else {
        if (!trailing_slash_) {
          path_ << '/';
        }
        path_ << component;
      }
      trailing_slash_ = component.back() == '/';
    }
    return *this;
  }

 private:
  bool trailing_slash_ = true;
  std::ostringstream path_;
};

// Tracks persistent root and local paths.
class Path : public ThreadCapture<Path> {
 public:
  static std::string Root() {
    if (GetCurrent()) {
      return GetCurrent()->RootPath();
    } else {
      return "";
    }
  }

  static std::string Working() {
    if (GetCurrent()) {
      PathBuilder builder;
      builder.Add(Root());
      GetCurrent()->AppendLocalPath(&builder);
      return builder.String();
    } else {
      return Root();
    }
  }

 protected:
  virtual std::string RootPath() const = 0;
  virtual void AppendLocalPath(PathBuilder* builder) const = 0;

  static std::string DelegateRootPath(const Path& path) {
    return path.RootPath();
  }

  static void DelegateLocalPath(const Path& path, PathBuilder* builder) {
    path.AppendLocalPath(builder);
  }
};

// Sets the root path, which persists while the instance is in scope.
class InRootPath : public Path {
 public:
  InRootPath(const std::string& root_path)
      : root_path_(root_path), capture_to_(this) {}

 protected:
  std::string RootPath() const override { return root_path_; }

  void AppendLocalPath(PathBuilder* builder) const override {
    assert(builder);
    const auto previous = capture_to_.Previous();
    if (previous) {
      DelegateLocalPath(*previous, builder);
    }
  }

 private:
  const std::string root_path_;
  const ScopedCapture capture_to_;
};

// Sets the local path relative to the current root + local path.
class InLocalPath : public Path {
 public:
  InLocalPath(const std::string& local_path)
      : local_path_(local_path), capture_to_(this) {}

 protected:
  std::string RootPath() const override {
    const auto previous = capture_to_.Previous();
    return previous ? DelegateRootPath(*previous) : "";
  }

  void AppendLocalPath(PathBuilder* builder) const override {
    assert(builder);
    const auto previous = capture_to_.Previous();
    if (previous) {
      DelegateLocalPath(*previous, builder);
    }
    builder->Add(local_path_);
  }

 private:
  const std::string local_path_;
  const ScopedCapture capture_to_;
};

// Simulates installing project binaries.
void InstallBin(const std::vector<std::string>& targets) {
  InLocalPath file("bin");
  for (const auto& target : targets) {
    InLocalPath file(target);
    std::cerr << "Installing binary " << Path::Working() << std::endl;
  }
}

// Simulates installing project libraries.
void InstallLib(const std::vector<std::string>& targets) {
  InLocalPath file("lib");
  for (const auto& target : targets) {
    InLocalPath file(target);
    std::cerr << "Installing library " << Path::Working() << std::endl;
  }
}

// Simulates installing the project.
void InstallProjectIn(const std::string& path) {
  InRootPath root(path);
  std::cerr << "Installing project in " << Path::Root() << std::endl;
  InstallBin({"binary1", "binary2"});
  InstallLib({"lib1.so", "lib2.so"});
}

int main() { InstallProjectIn("/usr/local"); }
