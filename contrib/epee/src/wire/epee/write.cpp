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

#include "write.h"

#include <limits>
#include "serialization/wire/error.h"

namespace wire
{
  namespace
  {
    template<typename T>
    void pack_varint(epee::byte_stream& bytes, const std::uint8_t tag, T value)
    {
      value <<= 2;
      value |= tag;
      bytes.write(reinterpret_cast<const char*>(std::addressof(value)), sizeof(value));
    }

    template<typename T>
    constexpr T varint_limit() noexcept
    {
      return std::numeric_limits<T>::max() >> 2;
    }
  }

  void epee_writer::write_tag(const std::uint8_t tag)
  {
    if (array_count_)
    {
      bytes_.put(tag | SERIALIZE_FLAG_ARRAY);
      write_varint(array_count_.get());
      array_count_.reset();
    }
    else
      bytes_.put(tag);
    needs_tag_ = false;
  }

  void epee_writer::write_varint(const std::size_t value)
  {
    if (value <= varint_limit<std::uint8_t>())
      pack_varint(bytes_, PORTABLE_RAW_SIZE_MARK_BYTE, std::uint8_t(value));
    else if (value <= varint_limit<std::uint16_t>())
      pack_varint(bytes_, PORTABLE_RAW_SIZE_MARK_WORD, std::uint16_t(value));
    else if (value <= varint_limit<std::uint32_t>())
      pack_varint(bytes_, PORTABLE_RAW_SIZE_MARK_DWORD, std::uint32_t(value));
    else if (value <= varint_limit<std::uint64_t>())
      pack_varint(bytes_, PORTABLE_RAW_SIZE_MARK_INT64, std::uint64_t(value));
    else
      WIRE_DLOG_THROW_(error::epee::varint_size);
  }

  epee_writer::epee_writer(epee::byte_stream&& bytes)
    : writer(), bytes_(std::move(bytes)), array_count_(), needs_tag_(false)
  {}

  epee_writer::~epee_writer() noexcept
  {}

  epee::byte_stream epee_writer::take_buffer()
  {
    epee::byte_stream out{std::move(bytes_)};
    bytes_.clear();
    array_count_.reset();
    needs_tag_ = false;
    return out
  }

  epee::byte_slice epee_writer::take_bytes()
  {
    return epee::byte_slice{take_buffer()};
  }

  void epee_writer::integer(const int source)
  {
    write_arithmetic(source);
  }
  void epee_writer::integer(const std::intmax_t source)
  {
    write_arithmetic(source);
  }
  void epee_writer::unsigned_integer(const unsigned source)
  {
    write_arithmetic(source);
  }
  void epee_writer::unsigned_integer(const std::uintmax_t source)
  {
    write_arithmetic(source);
  }
  void epee_writer::real(const double source)
  {
    write_arithmetic(source);
  }

  void epee_writer::string(const boost::string_ref source)
  {
    binary({reinterpret_cast<const std::uint8_t*>(source.data()), source.size()});
  }
  void epee_writer::binary(epee::span<const std::uint8_t> source)
  {
    if (needs_tag_)
      write_tag(SERIALIZE_TYPE_STRING);
    write_varint(source.size());
    bytes_.write(source);
  }

  void epee_writer::start_array(const std::size_t count, const type tag)
  {
    if (array_count_) // array of arrays
      write_tag(SERIALIZE_TYPE_ARRAY);

    needs_tag_ = true;
    array_count_.emplace(count);
  }
  void epee_writer::end_array()
  {}

  void epee_writer::start_object(const std::size_t count)
  {
    if (needs_tag_)
      write_tag(SERIALIZE_TYPE_OBJECT);
    write_varint(count);
  }
  void epee_writer::key(const boost::string_ref str)
  {
    if (std::numeric_limits<std::uint8_t>::max() < str.size())
      WIRE_DLOG_THROW(error::epee::key_size, "key size is too long");

    bytes_.put(std::uint8_t(str.size()));
    bytes_.write(str.data(), str.size());
    needs_tag_ = true;
  }
  void epee_writer::end_object()
  {}
} // wire
