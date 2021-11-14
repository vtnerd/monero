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
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

#include "byte_slice.h"
#include "byte_stream.h"
#include "int-util.h"
#include "serialization/wire/error.h"
#include "serialization/wire/field.h"
#include "serialization/wire/read.h"
#include "serialization/wire/traits.h"
#include "storages/portable_storage_bin_utils.h"
#include "span.h"

//! A required field, where the array contents are written as a single binary blob
#define WIRE_FIELD_ARRAY_AS_BLOB(name)                                  \
  ::wire::field( #name , ::wire::array_as_blob(std::ref( self . name )))

namespace wire
{
  /*! A wrapper that tells `wire::writer`s` and `wire::reader`s to encode a
    container as a single binary blob.

    `container_type` is `T` with optional `std::reference_wrapper` removed.
    `container_type` concept requirements:
      * `typedef` `value_type` that specifies inner type.
      * `std::is_pod<value_type>::value` must be true.
    Additional concept requirements for `container_type` when reading:
      * must have `clear()` method that removes all elements (`size() == 0`).
      * must have `emplace_back()` method that default initializes new element
      * must have `back()` method that retrieves last element by reference.
    Additional concept requirements for `container_type` when writing:
      * must work with foreach loop (`std::begin` and `std::end`).
      * must have `size()` method that returns number of elements. */
  template<typename T>
  struct array_as_blob_
  {
    using container_type = typename unwrap_reference<T>::type;
    using value_type = typename container_type::value_type;
    static constexpr std::size_t value_size() noexcept { return sizeof(value_type); }
    static_assert(std::is_pod<value_type>::value, "container value must be POD");

    T container;

    constexpr const container_type& get_container() const noexcept { return container; }
    container_type& get_container() noexcept { return container; }
  };

  template<typename T>
  inline array_as_blob_<T> array_as_blob(T value)
  {
    return {std::move(value)};
  }

#if BYTE_ORDER == LITTLE_ENDIAN
  //! Optimization when `T` has `resize()` and `data()` functions.
  template<typename T>
  inline auto read_as_blob(epee::byte_slice source, T& dest) -> decltype(dest.resize(0), dest.data())
  {
    using value_type = typename T::value_type;
    dest.resize(source.size() / sizeof(value_type));
    std::memcpy(dest.data(), source.data(), source.size());
    return dest.data();
  }

  //! Optimization when `T` has `data() function.
  template<typename W, typename T>
  inline auto write_as_blob(W& dest, const T& source) -> decltype(source.data())
  {
    using value_type = typename T::value_type;
    dest.binary({reinterpret_cast<const std::uint8_t*>(source.data()), source.size() * sizeof(value_type)});
    return source.data();
  }
#endif // LITTLE ENDIAN

  //! Default implmenetation when `T` does not have `resize()` and `data()` functions.
  template<typename T, typename... Ignored>
  inline void read_as_blob(epee::byte_slice source, T& dest, const Ignored&...)
  {
    using value_type = typename T::value_type;
    dest.clear();
    wire::reserve(dest, source.size() / sizeof(value_type));
    while (!source.empty())
    {
      dest.emplace_back();
      std::memcpy(std::addressof(dest.back()), source.data(), sizeof(value_type));
      dest.back() = CONVERT_POD(dest.back());
      source.remove_prefix(sizeof(value_type));
    }
  }

  //! Default implementation when `T` does not have `data()` function.
  template<typename W, typename T, typename... Ignored>
  inline void write_as_blob(W& dest, const T& source, const Ignored&...)
  {
    using value_type = typename T::value_type;
    epee::byte_stream bytes{};
    bytes.reserve(sizeof(value_type) * source.size());
    for (auto elem : source)
    {
      elem = CONVERT_POD(elem);
      bytes.write(reinterpret_cast<const char*>(std::addressof(elem)), sizeof(elem));
    }
    dest.binary(epee::to_span(bytes));
  }

  template<typename R, typename T>
  inline void read_bytes(R& source, array_as_blob_<T>& dest)
  {
    static_assert(array_as_blob_<T>::value_size() != 0, "divide by zero and no progress below");
    epee::byte_slice bytes = source.binary();
    if (bytes.size() % dest.value_size() != 0)
      WIRE_DLOG_THROW_(error::schema::fixed_binary);
    read_as_blob(std::move(bytes), dest.get_container());
  }

  template<typename W, typename T>
  inline void write_bytes(W& dest, const array_as_blob_<T>& source)
  {
    write_as_blob(dest, source.get_container());
  }
} // wire
