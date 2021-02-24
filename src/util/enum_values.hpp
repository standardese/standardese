// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_UTIL_ENUM_VALUES_HPP_INCLUDED
#define STANDARDESE_UTIL_ENUM_VALUES_HPP_INCLUDED

#include <type_traits>

namespace standardese {
namespace {

/// An iterable for the values of the enum `T`.
template <typename T, T count = T::count>
class enum_values {
  T at = static_cast<T>(0);
public:
  enum_values operator++() {
    at = static_cast<T>(static_cast<std::underlying_type_t<T>>(at) + 1);
    return *this;
  }
  T operator*() {
      return at;
  }
  enum_values begin() {
      return *this;
  }
  enum_values end() {
      enum_values end;
      end.at = count;
      return end;
  }
  bool operator!=(const enum_values& rhs) { return at != rhs.at; }
};


}
}

#endif
