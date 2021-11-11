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

#include <cstring>
#include <type_traits>
#include <utility>

#include "byte_slice.h"
#include "byte_stream.h"
#include "serialization/wire/error.h"
#include "serialization/wire/field.h"
#include "serialization/wire/read.h"
#include "serialization/wire/traits.h"
#include "serialization/wire/write.h"
#include "span.h"

//
// A set of "wrappers" that change how a value is read/written
//

//! An optional field that is omitted when a default value is used
#define WIRE_FIELD_DEFAULTED(name, default_)                            \
  ::wire::optional_field( #name , ::wire::defaulted(std::ref( self . name ), default_ ))

//! A required array field with read constraint. See `array_` for more info.
#define WIRE_FIELD_ARRAY(name, read_constraint)         \
  ::wire::field( #name , ::wire::array< read_constraint >(std::ref( self . name )))

//! A required field, where the array contents are written as a single binary blob
#define WIRE_FIELD_ARRAY_AS_BLOB(name)                                  \
  ::wire::field( #name , ::wire::array_as_blob(std::ref( self . name )))

namespace wire
{
  /*! A wrapper that tells `wire::writer`s to skip field generation when default
    value, and tells `wire::reader`s to use default value when field not present. */
  template<typename T, typename U>
  struct defaulted_
  {
    using value_type = typename unwrap_reference<T>::type;

    T value;
    U default_;

    //! \return `value` with `std::reference_wrapper` removed.
    constexpr const value_type& get_value() const noexcept { return value; }

    //! \return `value` with `std::reference_wrapper` removed.
    value_type& get_value() noexcept { return value; }

    //! \return `default_` with `std::reference_wrapper` removed.
    constexpr const value_type& get_default() const noexcept { return default_; }

    //! \return `default_` with `std::reference_wrapper` removed.
    value_type& get_default() noexcept { return default_; }


    //! \return `get_value() != get_default()`; concept requirement for optional wire type.
    constexpr explicit operator bool() const noexcept { return get_value() != get_default(); }

    //! \return `get_value()`; concept requirement for optional wire type.
    value_type& emplace() noexcept { return get_value(); }

    //! \return `get_value()`; concept requirement for optional wire type.
    constexpr const value_type& operator*() const noexcept { return get_value(); }

    //! \return `get_value()`; concept requirement for optional wire type.
    value_type& operator*() noexcept { return get_value(); }
  };

  //! Links `value` with `default_` when reading from object field (see `read_reset`).
  template<typename T, typename U>
  constexpr inline defaulted_<T, U> defaulted(T value, U default_)
  {
    return {std::move(value), std::move(default_)};
  }

  /* read/write functions not needed since `defaulted` meets the concept
     requirements for an optional type (optional fields are handled
     directly by the generic read/write code because the field name is omitted
     entirely when the value is "empty"). */

  template<typename R, typename T, typename U>
  inline void read_reset(const R&, defaulted_<T, U>& wrapper)
  {
    wrapper.get_value() = wrapper.get_default();
  }


  /*! A wrapper that ensures `T` is written as an array, with `C` constraints
    when reading (`max_element_count` or `min_element_size`). `C` can be `void`
    if write-only.

    `container_type` is `T` with optional `std::reference_wrapper` removed.
    `container_type` concept requirements:
      * `typedef` `value_type` that specifies inner type.
      * must have `size()` function that returns number of (`std::size_t`)
        elements.
    Additional concept requirements for `container_type` when reading:
      * must have `clear()` function that removes all elements (`size() == 0`).
      * must have `emplace_back()` function that default initializes new element
      * must have `back()` function that retrieves last element by reference.
      * must define `template<typename R> void read_reserve(const R&, container_type&, std::size_t)`
        in `wire` namespace or `container_type`'s namespace.
      * _should_ define `template<typename R> void read_reset(const R&, container_type&)`
        in `wire` namespace or `container_type`'s namespace if default
        construction frees memory.
    Additional concept requirements for `container_type` when writing:
       * must work with foreach loop (`std::begin` and `std::end`).

    \note Constraint is a templated type to help with compiler optimizations in
      `wire_read::array`. In most cases, at least some of the branches can be
      optimized away if the compiler can easily deduce the value at compile-time. */
  template<typename T, typename C = void>
  struct array_
  {
    using constraint = C;
    using container_type = typename unwrap_reference<T>::type;
    using value_type = typename container_type::value_type;
    static_assert(std::is_same<std::size_t, decltype(std::declval<container_type>().size())>::value, "size() function must return std::size_t");

    T container;

    //! \return `container` with `std::reference_wrapper` removed.
    constexpr const container_type& get_container() const noexcept { return container; }

    //! \return `container` with `std::reference_wrapper` removed.
    container_type& get_container() noexcept { return container; }
  };

  //! Treat `value` as an array when reading/writing, and constrain reading with `C` (optional).
  template<typename C = void, typename T = void>
  constexpr inline array_<T, C> array(T value)
  {
    return {std::move(value)};
  }

  template<typename R, typename T, typename C>
  inline void read_reserve(R& source, array_<T, C>& wrapper, const std::size_t count)
  {
    wire_read::reserve(source, wrapper.get_container(), count);
  }

  template<typename R, typename T, typename C>
  inline void read_reset(R& source, array_<T, C>& wrapper)
  {
    wire_read::reset(source, wrapper.get_container());
  }

  template<typename R, typename T, typename C>
  inline void read_bytes(const R&, const array_<T, C>&)
  {
    // see constraints directly above `array_` definition
    static_assert(std::is_same<R, void>::value, "array_ must have a read constraint for memory purposes");
  }
  template<typename R, typename T, std::size_t N>
  inline void read_bytes(R& source, array_<T, max_element_count<N>>& wrapper)
  {
    using array_type = array_<T, max_element_count<N>>;
    static_assert(array_type::constraint::check<array_type::value_type>(), "max reserve bytes exceeded for element");
    wire_read::array(source, wrapper.get_container(), min_element_size<0>{}, array_type::constraint{});
  }
  template<typename R, typename T, std::size_t N>
  inline void read_bytes(R& source, array_<T, min_element_size<N>>& wrapper)
  {
    using array_type = array_<T, min_element_size<N>>;
    static_assert(array_type::constraint::check<array_type::value_type>(), "max compression ratio exceeded for element");
    wire_read::array(source, wrapper.get_container(), array_type::constraint{});
  }

  template<typename W, typename T, typename C>
  inline void write_bytes(W& dest, const array_<T, C>& wrapper)
  {
    wire_write::array(dest, wrapper.get_container());
  }


  /*! A wrapper that tells `wire::writer`s` and `wire::reader`s to encode a
    container as a single binary blob.

    `container_type` is `T` with optional `std::reference_wrapper` removed.
    `container_type` concept requirements:
      * `typedef` `value_type` that specifies inner type.
    Additional concept requirements for `container_type` when reading:
      * must have `clear()` function that removes all elements (`size() == 0`).
      * must have `emplace_back()` function that default initializes new element
      * must have `back()` function that retrieves last element by reference.
    Additional concept requirements for `container_type` when writing:
      * must work with foreach loop (`std::begin` and `std::end`).
      * must have `size()` function that returns number of elements. */
  template<typename T>
  struct array_as_blob_
  {
    using container_type = typename unwrap_reference<T>::type;
    using value_type = typename container_type::value_type;
    static constexpr std::size_t value_size() noexcept { return sizeof(value_type); }

    T container;

    //! \return `container` with `std::reference_wrapper` removed.
    constexpr const container_type& get_container() const noexcept { return container; }

    //! \return `container` with `std::reference_wrapper` removed.
    container_type& get_container() noexcept { return container; }
  };

  template<typename T>
  constexpr inline array_as_blob_<T> array_as_blob(T value)
  {
    return {std::move(value)};
  }

  template<typename R, typename T>
  inline void read_reset(const R& source, array_as_blob_<T>& wrapper)
  {
    wire_read::reset(source, wrapper.get_container());
  }

#if BYTE_ORDER == LITTLE_ENDIAN
  // enable optimization if `T` has `resize` and `data()`
  template<typename T>
  auto read_as_blob(epee::byte_slice source, T& dest) -> decltype(dest.resize(0), dest.data())
  {
    using value_type = typename T::value_type;
    dest.resize(source.size() / sizeof(value_type));
    std::memcpy(dest.data(), source.data(), source.size());
    return dest.data();
  }

  // enable optimization if `T` has `data()`
  template<typename W, typename T>
  auto write_as_blob(W& dest, const T& source) -> decltype(source.data())
  {
    using value_type = typename T::value_type;
    dest.binary({reinterpret_cast<const std::uint8_t*>(source.data()), source.size() * sizeof(value_type)});
    return dest.data();
  }
#endif // LITTLE ENDIAN

  // Parameter packs take lower precedence (above preferred if `T` has extra functions)

  template<typename T, typename... Ignored>
  void read_as_blob(epee::byte_slice source, T& dest, const Ignored&...)
  {
    using value_type = typename T::value_type;
    dest.clear();
    wire_read::read_reserve(source, dest, bytes.size() / sizeof(value_type));
    while (!bytes.empty())
    {
      dest.emplace_back();
      std::memcpy(std::addressof(dest.back()), bytes.data(), sizeof(value_type));
      dest.back() = CONVERT_POD(dest.back());
      bytes.remove_prefix(sizeof(value_type));
    }
  }

  template<typename W, typename T, typename... Ignored>
  void write_as_blob(W& dest, const T& source, const Ignored&...)
  {
    using value_type = typename T::value_type;
    epee::byte_stream bytes{};
    bytes.reserve(sizeof(value_type) * source.size());
    for (const auto elem : source)
    {
      elem = CONVERT_POD(elem);
      bytes.write(reinterpret_cast<const char*>(std::addressof(elem)), sizeof(elem));
    }

    dest.binary(epee::to_span(bytes));
  }

  template<typename R, typename T>
  void read_bytes(R& source, array_as_blob_<T>& dest)
  {
    static_assert(dest.value_size() != 0, "divide by zero and no progress below");
    epee::byte_slice bytes = source.binary();
    if (bytes.size() % dest.value_size() != 0)
      WIRE_DLOG_THROW_(error::schema::fixed_binary);

    read_as_blob(std::move(bytes), dest.get_container());
  }

  template<typename W, typename T>
  void write_bytes(W& dest, const array_as_blob_<T>& source)
  {
    write_as_blob(dest, source.get_container());
  }
} // wire
