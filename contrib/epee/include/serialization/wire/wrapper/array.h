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
#include <utility>

#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"

/*! An array field with read constraint. See `array_` for more info. All (empty)
  arrays were "optional" (omitted) historically in epee, so this matches prior
  behavior. */
#define WIRE_FIELD_ARRAY(name, read_constraint)         \
  ::wire::optional_field( #name , ::wire::array< read_constraint >(std::ref( self . name )))

namespace wire
{
  /*! A wrapper that ensures `T` is written as an array, with `C` constraints
    when reading (`max_element_count` or `min_element_size`). `C` can be `void`
    if write-only.

    This wrapper meets the requirements for an optional field; `wire::field`
    and `wire::optional_field` determine whether an empty array must be
    encoded on the wire. Historically, empty arrays were always omitted on
    the wire (a defacto optional field).

    The `is_array` trait can also be used, but is always treated as an optional
    field.

    `container_type` is `T` with optional `std::reference_wrapper` removed.
    `container_type` concept requirements:
      * `typedef` `value_type` that specifies inner type.
      * must have `size()` method that returns number of elements.
    Additional concept requirements for `container_type` when reading:
      * must have `clear()` method that removes all elements (`size() == 0`).
      * must have `emplace_back()` method that default initializes new element
      * must have `back()` method that retrieves last element by reference.
    Additional concept requirements for `container_type` when writing:
      * must work with foreach loop (`std::begin` and `std::end`). */
  template<typename T, typename C = void>
  struct array_
  {
    using constraint = C;
    using container_type = unwrap_reference_t<T>;
    using value_type = typename container_type::value_type;

    T container;

    constexpr const container_type& get_container() const noexcept { return container; }
    container_type& get_container() noexcept { return container; }

    // concept requirements for optional fields

    explicit operator bool() const noexcept { return !get_container().empty(); }
    array_& emplace() noexcept { return *this; }

    array_& operator*() noexcept { return *this; }
    const array_& operator*() const noexcept { return *this; }

    void reset() { get_container().clear(); }
  };

  //! Treat `value` as an array when reading/writing, and constrain reading with `C`.
  template<typename C = void, typename T = void>
  inline constexpr array_<T, C> array(T value)
  {
    return {std::move(value)};
  }
} // wire
