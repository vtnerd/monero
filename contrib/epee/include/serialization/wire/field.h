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

#include <functional>
#include <utility>

#include "serialization/wire/traits.h"

//! A required field has the same key name and C/C++ name
#define WIRE_FIELD(name)                                \
  ::wire::field( #name , std::ref( self . name ))

//! A required field has the same key name and C/C++ name AND is cheap to copy (faster output).
#define WIRE_FIELD_COPY(name)                   \
  ::wire::field( #name , self . name )

//! The optional field has the same key name and C/C++ name
#define WIRE_OPTIONAL_FIELD(name)                               \
  ::wire::optional_field( #name , std::ref( self . name ))

namespace wire
{
  /*! Links `name` to a `value` for object serialization.

    `value_type` is `T` with optional `std::reference_wrapper` removed.
    `value_type` needs a `read_bytes` function when parsing with a
    `wire::reader` - see `read.h` for more info. `value_type` needs a
    `write_bytes` function when parsing with a `wire::writer` - see `write.h`
    for more info.

    Additional concept requirements for `value_type` when `Required == false`:
      * must have an `operator*()` function that returns some other type (i.e.
        `return *this` not allowed)
      * must have a conversion to bool function that returns true when
        `operator*()` is safe to call (and implicitly when the associated field
        should be written as opposed to skipped/omitted).
    Additional concept requirements for `value_type` when `Required == false`
    when reading:
      * must have an `emplace()` method that ensures `operator*()` is safe to call.
      * must have a `reset()` method to indicate a field was skipped/omitted.

    If a standard type needs custom serialization, one "trick":
    ```
    struct custom_tag{};
    void read_bytes(wire::reader&, boost::fusion::pair<custom_tag, std::string&>)
    { ... }
    void write_bytes(wire::writer&, boost::fusion::pair<custom_tag, const std::string&>)
    { ... }

    template<typename F, typename T>
    void object_map(F& format, T& self)
    {
      wire::object(format,
        wire::field("foo", boost::fusion::make_pair<custom_tag>(std::ref(self.foo)))
      );
    }
    ```

    Basically each input/output format needs a unique type so that the compiler
    knows how to "dispatch" the read/write calls. */
  template<typename T, bool Required>
  struct field_
  {
    using value_type = typename unwrap_reference<T>::type;
    static constexpr bool is_required() noexcept { return Required; }
    static constexpr std::size_t count() noexcept { return 1; }

    const char* name;
    T value;

    constexpr const value_type& get_value() const noexcept { return value; }
    value_type& get_value() noexcept { return value; }
  };

  //! Links `name` to `value`. Use `std::ref` if de-serializing.
  template<typename T>
  constexpr inline field_<T, true> field(const char* name, T value)
  {
    return {name, std::move(value)};
  }

  //! Links `name` to optional `value`. Use `std::ref` if de-serializing.
  template<typename T>
  constexpr inline field_<T, false> optional_field(const char* name, T value)
  {
    return {name, std::move(value)};
  }


  template<typename T>
  inline constexpr bool available(const field_<T, true>&) noexcept
  {
    return true;
  }
  template<typename T>
  inline bool available(const field_<T, false>& elem)
  {
    return bool(elem.get_value());
  }

  // example usage : `wire::sum(std::size_t(wire::available(fields))...)`

  inline constexpr int sum() noexcept
  {
    return 0;
  }
  template<typename T, typename... U>
  inline constexpr T sum(const T head, const U... tail) noexcept
  {
    return head + sum(tail...);
  }
}
