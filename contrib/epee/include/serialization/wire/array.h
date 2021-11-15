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
#include <functional>
#include <type_traits>
#include <utility>

#include "serialization/wire/field.h"
#include "serialization/wire/read.h"
#include "serialization/wire/traits.h"
#include "serialization/wire/write.h"

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
    using container_type = typename unwrap_reference<T>::type;
    using value_type = typename container_type::value_type;

    T container;

    constexpr const container_type& get_container() const noexcept { return container; }
    container_type& get_container() noexcept { return container; }

    // concept requirements for optional fields

    explicit operator bool() const noexcept { return !get_container().empty(); }
    container_type& emplace() noexcept { return get_container(); }

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


  //! No valid constraint given for array read, compile error
  template<typename R, typename T, typename C>
  inline void read_bytes(const R&, const array_<T, C>&)
  {
    // see constraints directly above `array_` definition
    static_assert(std::is_same<R, void>::value, "array_ must have a read constraint for memory purposes");
  }

  //! Read array with a max element count constraint
  template<typename R, typename T, std::size_t N>
  inline void read_bytes(R& source, array_<T, max_element_count<N>>& wrapper)
  {
    using array_type = array_<T, max_element_count<N>>;
    using value_type = typename array_type::value_type;
    static_assert(array_type::constraint::template check<value_type>(), "max reserve bytes exceeded for element");
    wire_read::array(source, wrapper.get_container(), min_element_size<0>{}, typename array_type::constraint{});
  }

  //! Read array with a min element size constraint (constraint is relative to archive size)
  template<typename R, typename T, std::size_t N>
  inline void read_bytes(R& source, array_<T, min_element_size<N>>& wrapper)
  {
    using array_type = array_<T, min_element_size<N>>;
    using value_type = typename array_type::value_type;
    static_assert(array_type::constraint::template check<value_type>(), "max compression ratio exceeded for element");
    wire_read::array(source, wrapper.get_container(), typename array_type::constraint{});
  }

  //! Write array
  template<typename W, typename T, typename C>
  inline void write_bytes(W& dest, const array_<T, C>& wrapper)
  {
    wire_write::array(dest, wrapper.get_container());
  }
} // wire
