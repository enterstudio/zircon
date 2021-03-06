// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fbl/type_support.h>
#include <stdlib.h>

namespace fbl {

template<class T>
constexpr const T& min(const T& a, const T& b) {
    return (b < a) ? b : a;
}

template<class T>
constexpr const T& max(const T& a, const T& b) {
    return (a < b) ? b : a;
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

// is_pow2
//
// Test to see if an unsigned integer type is an exact power of two or
// not.  Note, this needs to use a helper struct because we are not
// allowed to partially specialize functions (because C++).
namespace internal {
template <typename T, typename Enable = void> struct IsPow2Helper;

template <typename T>
struct IsPow2Helper<T, typename enable_if<is_unsigned_integer<T>::value>::type> {
    static constexpr bool is_pow2(T val) {
        return (val != 0) && (((val - 1u) & val) == 0);
    }
};
}

// is_pow2<T>(T val)
//
// Tests to see if val (which may be any unsigned integer type) is a power of 2
// or not.  0 is not considered to be a power of 2.
//
template <typename T>
constexpr bool is_pow2(T val) {
    return internal::IsPow2Helper<T>::is_pow2(val);
}

// round_up rounds up val until it is divisible by multiple.
// Zero is divisible by all multiples.
template<class T, class U,
         class = typename enable_if<is_unsigned_integer<T>::value>::type,
         class = typename enable_if<is_unsigned_integer<U>::value>::type>
constexpr const T round_up(const T& val, const U& multiple) {
    return val == 0 ? 0 :
            is_pow2<U>(multiple) ? (val + (multiple - 1)) & ~(multiple - 1) :
                ((val + (multiple - 1)) / multiple) * multiple;
}

// round_down rounds down val until it is divisible by multiple.
// Zero is divisible by all multiples.
template<class T, class U,
         class = typename enable_if<is_unsigned_integer<T>::value>::type,
         class = typename enable_if<is_unsigned_integer<U>::value>::type>
constexpr const T round_down(const T& val, const U& multiple) {
    return val == 0 ? 0 :
            is_pow2<U>(multiple) ? val & ~(multiple - 1) :
                (val / multiple) * multiple;
}

// Returns a pointer to the first element that is not less than |value|, or
// |last| if no such element is found.
//
// Similar to <http://en.cppreference.com/w/cpp/algorithm/lower_bound>
template<class T, class U>
const T* lower_bound(const T* first, const T* last, const U& value) {
    while (first < last) {
        const T* probe = first + (last - first) / 2;
        if (*probe < value) {
            first = probe + 1;
        } else {
            last = probe;
        }
    }
    return last;
}

// Returns a pointer to the first element that is not less than |value|, or
// |last| if no such element is found.
//
// |comp| is used to compare the elements rather than operator<.
//
// Similar to <http://en.cppreference.com/w/cpp/algorithm/lower_bound>
template<class T, class U, class Compare>
const T* lower_bound(const T* first, const T* last, const U& value, Compare comp) {
    while (first < last) {
        const T* probe = first + (last - first) / 2;
        if (comp(*probe, value)) {
            first = probe + 1;
        } else {
            last = probe;
        }
    }
    return last;
}

template <typename T, size_t N>
constexpr size_t count_of(T const(&)[N]) {
    return N;
}

// 2017-12-10: On ARM/release/GCC, a div(mod)-based implementation runs at 2x
// the speed of subtract-based ones, with no O(n) scaling as input values grow.
// For x86-64/release/GCC, sub-based methods are initially 20-40% faster (depending
// on int type) but scale linearly; they are comparable for values 100000-200000.
//
// gcd (greatest common divisor) returns the largest non-negative integer that cleanly
// divides both inputs. Inputs are unsigned integers. gcd(x,0)=x; gcd(x,1)=1
template <typename T, class = typename enable_if<is_unsigned_integer<T>::value>::type>
T gcd(T first, T second) {
    // If function need not support uint8 or uint16, static_casts can be removed
    while (second != 0) {
        first = static_cast<T>(first % second);
        if (first == 0) {
            return second;
        }
        second = static_cast<T>(second % first);
    }

    return first;
}

// lcm (least common multiple) returns the smallest non-negative integer that is
// cleanly divided by both inputs.
// Inputs are unsigned integers. lcm(x,0)=0; lcm(x,1)=x
template <typename T, class = typename enable_if<is_unsigned_integer<T>::value>::type>
T lcm(T first, T second) {
    if (first == 0 && second == 0) {
        return 0;
    }

    // If function need not support uint8 or uint16, static_cast can be removed
    return static_cast<T>((first / gcd(first, second)) * second);
}

}  // namespace fbl
