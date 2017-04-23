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

// This example demonstrates how the framework can be used to mock resources
// that are otherwise handled by non-trivial default functionality.

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "thread-capture.h"

using capture_thread::ThreadCapture;

// Handles the creation of file objects. Unless an instance of an implementation
// is in scope, uses a default file-open operation. Implementations can override
// if/how files are opened.
class FileFactory : public ThreadCapture<FileFactory> {
 public:
  // Returns an open file, or nullptr.
  static std::unique_ptr<std::istream> ReadFile(const std::string& filename) {
    if (GetCurrent()) {
      return GetCurrent()->GetReadStream(filename);
    } else {
      return std::unique_ptr<std::istream>(new std::ifstream(filename));
    }
  }

 protected:
  FileFactory() = default;
  virtual ~FileFactory() = default;

  // Overrides how files are opened.
  virtual std::unique_ptr<std::istream> GetReadStream(
      const std::string& filename) = 0;
};

// Replaces the default file-open behavior of FileFactory when in scope.
class FileMocker : public FileFactory {
 public:
  FileMocker() : capture_to_(this) {}

  // Registers a string-backed mock file that's used in place of an actual file
  // on the filesystem.
  void MockFile(std::string filename, std::string content) {
    files_.emplace(std::move(filename), std::move(content));
  }

 protected:
  std::unique_ptr<std::istream> GetReadStream(
      const std::string& filename) override {
    const auto mock = files_.find(filename);
    if (mock == files_.end()) {
      return nullptr;
    } else {
      return std::unique_ptr<std::istream>(
          new std::istringstream(mock->second));
    }
  }

 private:
  std::unordered_map<std::string, std::string> files_;
  const ScopedCapture capture_to_;
};

// Some arbitrary function whose API we can't change to add mocking capability.
int OpenConfigAndCountWords() {
  const auto input = FileFactory::ReadFile("CMakeLists.txt");
  if (!input) {
    return -1;
  } else {
    int count = 0;
    std::string sink;
    while ((*input >> sink)) {
      ++count;
    }
    return count;
  }
}

int main() {
  std::cerr << "Word count *without* mock: " << OpenConfigAndCountWords()
            << std::endl;

  {
    // Only overrides FileFactory::ReadFile while in scope.
    FileMocker mocker;

    std::cerr << "Word count with missing file: " << OpenConfigAndCountWords()
              << std::endl;

    mocker.MockFile("CMakeLists.txt", "one two three");
    std::cerr << "Word count *with* mock: " << OpenConfigAndCountWords()
              << std::endl;
  }

  std::cerr << "Word count *without* mock: " << OpenConfigAndCountWords()
            << std::endl;
}
