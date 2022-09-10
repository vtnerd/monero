// Copyright (c) 2021, The Monero Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cstdint>
#include <type_traits>

#define WIRE_DECLARE_BLOB(type)                 \
  template<>                                    \
  struct is_blob<type>                          \
    : std::true_type                            \
  {};

#define WIRE_DECLARE_BLOB_NS(type)              \
  namespace wire { WIRE_DECLARE_BLOB(type) }

namespace wire
{
  template<typename T>
  struct unwrap_reference
  {
    using type = T;
  };

  template<typename T>
  struct unwrap_reference<std::reference_wrapper<T>>
  {
    using type = T;
  };

  //! Mark `T` as an array for writing purposes only. See `array_` in `wrapper/array.h`.
  template<typename T>
  struct is_array : std::false_type
  {};

  /*! Mark `T` as fixed binary data for reading+writing. Concept requirements
      for reading:
        * `T` must be compatible with `epee::as_mut_byte_span` (`std::is_pod<T>`
          and no padding).
      Concept requirements for writing:
        * `T` must be compatible with `epee::as_byte_span` (std::is_pod<T>` and
          no padding). */
  template<typename T>
  struct is_blob : std::false_type
  {};

  //! A constraint for `wire_read::array` where a max of `N` elements can be read.
  template<std::size_t N>
  struct max_element_count
    : std::integral_constant<std::size_t, N>
  {
    // The threshold is low - min_element_size is a better constraint metric
    static constexpr std::size_t max_bytes() noexcept { return 512 * 1024; } // 512 KiB

    //! \return True if `N` C++ objects of type `T` are below `max_bytes()` threshold.
    template<typename T>
    static constexpr bool check() noexcept
    {
      return N <= (max_bytes() / sizeof(T));
    }
  };

  //! A constraint for `wire_read::array` where each element must be at least `N` bytes (in any archive format).
  template<std::size_t N>
  struct min_element_size
    : std::integral_constant<std::size_t, N>
  {
    static constexpr std::size_t max_ratio() noexcept { return 4; }

    //! \return True if C++ object of type `T` with minimum wire size `N` is below `max_ratio()`.
    template<typename T>
    static constexpr bool check() noexcept
    {
      return N ? (sizeof(T) / N) <= max_ratio() : false;
    }
  };

  //! If container has no `reserve(0)` function, this empty function is used
  template<typename... T>
  inline void reserve(const T&...) noexcept
  {}

  //! Container has `reserve(std::size_t)` function, use it
  template<typename T>
  inline auto reserve(T& container, const std::size_t count) -> decltype(container.reserve(count))
  {
    return container.reserve(count);
  }
} // wire
