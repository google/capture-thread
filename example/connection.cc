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

// This example demonstrates sharing a client connection (e.g., a socket) within
// the thread so that it doesn't need to be passed around to all functions that
// require access to it.

#include <cassert>
#include <iostream>
#include <string>

#include "thread-capture.h"

using capture_thread::ThreadCapture;

// Serves as a singleton client connection within the thread. The default
// behavior is to act as a disconnected client.
class ClientConnection : public ThreadCapture<ClientConnection> {
 public:
  static bool IsActive() {
    if (GetCurrent()) {
      return GetCurrent()->CheckConnection();
    } else {
      return false;
    }
  }

  static bool Send(const std::string& message) {
    if (GetCurrent()) {
      return GetCurrent()->SendMessage(message);
    } else {
      return false;
    }
  }

  static bool Receive(std::string* message) {
    if (GetCurrent()) {
      return GetCurrent()->ReceiveMessage(message);
    } else {
      return false;
    }
  }

 protected:
  virtual bool CheckConnection() = 0;
  virtual bool SendMessage(const std::string& message) = 0;
  virtual bool ReceiveMessage(std::string* message) = 0;
};

// Provides client functionality from stdin and stdout while in scope.
class ClientFromStandardStreams : public ClientConnection {
 public:
  ClientFromStandardStreams() : capture_to_(this) {
    std::cerr << "Opening ClientFromStandardStreams connection." << std::endl;
  }

  ~ClientFromStandardStreams() {
    std::cerr << "Closing ClientFromStandardStreams connection." << std::endl;
  }

 protected:
  bool CheckConnection() { return !!std::cout && !!std::cin; }

  bool SendMessage(const std::string& message) {
    return (std::cout << "*** Message: " << message << " ***" << std::endl);
  }

  bool ReceiveMessage(std::string* message) {
    assert(message);
    return std::getline(std::cin, *message, '\n');
  }

 private:
  const ScopedCapture capture_to_;
};

// This is a helper function to send to and recieve from the client.
bool PromptForInfo(const std::string& prompt, std::string* response) {
  assert(response);
  while (ClientConnection::IsActive()) {
    if (!ClientConnection::Send(prompt)) {
      return false;
    }
    if (!ClientConnection::Receive(response)) {
      return false;
    }
    if (!response->empty()) {
      return true;
    }
  }
  return false;
}

// This is the main routine to handle the lifetime of the connection.
void HandleConnection() {
  std::string name;
  if (!PromptForInfo("What is your name?", &name)) {
    std::cerr << "Connection closed without providing a name." << std::endl;
  } else {
    ClientConnection::Send("Your name is supposedly \"" + name + "\".");
  }
}

int main() {
  // While in scope, client will serve as the ClientConnection for all calls.
  ClientFromStandardStreams client;
  HandleConnection();
}
