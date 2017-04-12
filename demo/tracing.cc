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

#include "tracing.h"

namespace demo {

// static
std::string Tracing::GetContext() {
  Formatter formatter;
  ReverseTrace(GetCurrent(), &formatter);
  return formatter.String();
}

// static
void Tracing::ReverseTrace(const Tracing* tracer, Formatter* formatter) {
  assert(formatter);
  if (tracer) {
    const auto previous = tracer->cross_and_capture_to_.Previous();
    ReverseTrace(previous, formatter);
    if (previous) {
      *formatter << ":";
    }
    *formatter << tracer->name();
  }
}

}  // namespace demo
