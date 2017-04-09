#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "thread-capture.h"

class FileAdapter : public ThreadCapture<FileAdapter> {
 public:
  FileAdapter() : capture_to_(this) {}

  static std::unique_ptr<std::istream> ReadFile(const std::string& filename) {
    if (GetCurrent()) {
      return GetCurrent()->GetReadStream(filename);
    } else {
      return std::unique_ptr<std::istream>(new std::ifstream(filename));
    }
  }

 protected:
  virtual std::unique_ptr<std::istream>
      GetReadStream(const std::string& filename) = 0;

 private:
  const ScopedCapture capture_to_;
};

class FileMocker : public FileAdapter {
 public:
  void MockFile(std::string filename,  std::string content) {
    files_.emplace(std::move(filename), std::move(content));
  }

 protected:
  std::unique_ptr<std::istream>
      GetReadStream(const std::string& filename) override {
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
};

int OpenConfigAndCountWords() {
  const auto input = FileAdapter::ReadFile("CMakeLists.txt");
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
  std::cerr << "Word count *without* mock: "
            << OpenConfigAndCountWords() << std::endl;
  {
    FileMocker mocker;
    mocker.MockFile("CMakeLists.txt", "one two three");
    std::cerr << "Word count *with* mock: "
              << OpenConfigAndCountWords() << std::endl;
  }
  std::cerr << "Word count *without* mock: "
            << OpenConfigAndCountWords() << std::endl;
}
