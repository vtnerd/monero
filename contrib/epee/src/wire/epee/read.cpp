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

#include "serialization/wire/epee/read.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "serialization/wire/error.h"
#include "serialization/wire/epee/error.h"
#include "storages/portable_storage.h"
#include "storages/portable_storage_base.h"
#include "storages/portable_storage_bin_utils.h"

namespace
{
  constexpr std::size_t min_array_size = 3; //!< Excluding type tag -> type_tag + varint_tag + 1-byte varint
  constexpr std::size_t min_object_size = 2;//!< Excluding type tag -> varint_tag + 1-byte varint
  constexpr std::size_t min_string_size = 2;//!< Excluding type tag -> varint_tag + 1-byte varint

  std::size_t min_wire_size(const std::uint8_t tag)
  {
    // never return 0 -> see `skip_fixed(...) and `start_array()`
    switch (tag)
    {
    case SERIALIZE_TYPE_ARRAY:  return min_array_size;
    case SERIALIZE_TYPE_BOOL:   return sizeof(bool);
    case SERIALIZE_TYPE_DUOBLE: return sizeof(double);
    case SERIALIZE_TYPE_INT8:   return sizeof(std::int8_t);
    case SERIALIZE_TYPE_INT16:  return sizeof(std::int64_t);
    case SERIALIZE_TYPE_INT32:  return sizeof(std::int32_t);
    case SERIALIZE_TYPE_INT64:  return sizeof(std::int64_t);
    case SERIALIZE_TYPE_OBJECT: return min_object_size;
    case SERIALIZE_TYPE_STRING: return min_string_size;
    case SERIALIZE_TYPE_UINT8:  return sizeof(std::uint8_t);
    case SERIALIZE_TYPE_UINT16: return sizeof(std::uint16_t);
    case SERIALIZE_TYPE_UINT32: return sizeof(std::uint32_t);
    case SERIALIZE_TYPE_UINT64: return sizeof(std::uint64_t);
    default:
      break;
    }
    WIRE_DLOG_THROW_(wire::error::epee::invalid_tag);
  }
} // anonymous

namespace wire
{
  template<typename T>
  T epee_reader::read()
  {
    static_assert(std::is_pod<T>::value, "only pod types allowed");
    if (remaining_.remove_prefix(sizeof(T)) != sizeof(T))
      WIRE_DLOG_THROW(error::epee::not_enough_bytes, "fixed size arithmetic of " << sizeof(T) << " bytes");
    T out{};
    std::memcpy(std::addressof(out), remaining_.data() - sizeof(T), sizeof(T));
    return CONVERT_POD(out);
  }

  std::uint8_t epee_reader::read_tag()
  {
    return read<std::uint8_t>();
  }

  std::size_t epee_reader::read_varint()
  {
    if (remaining_.empty())
      WIRE_DLOG_THROW(error::epee::not_enough_bytes, "varint tag");

    std::size_t out = 0;
    switch (remaining_[0] & PORTABLE_RAW_SIZE_MARK_MASK)
    {
    case PORTABLE_RAW_SIZE_MARK_BYTE:  out = read<uint8_t>(); break;
    case PORTABLE_RAW_SIZE_MARK_WORD:  out = integer::convert_to<std::size_t>(read<uint16_t>()); break;
    case PORTABLE_RAW_SIZE_MARK_DWORD: out = integer::convert_to<std::size_t>(read<uint32_t>()); break;
    case PORTABLE_RAW_SIZE_MARK_INT64: out = integer::convert_to<std::size_t>(read<uint64_t>()); break;
    default:
      WIRE_DLOG_THROW_(error::epee::invalid_varint_type);
    }
    out >>= 2;
    return out;
  }

  boost::string_ref epee_reader::read_name()
  {
    const std::uint8_t length = read<std::uint8_t>();
    if (remaining_.remove_prefix(length) != length)
      WIRE_DLOG_THROW(error::epee::not_enough_bytes, "key name unavailable");
    return {reinterpret_cast<const char*>(remaining_.data()) - length, length};
  }

  epee::span<const std::uint8_t> epee_reader::raw(error::schema expected)
  {
    if (last_tag_ != SERIALIZE_TYPE_STRING)
      WIRE_DLOG_THROW_(expected);

    const std::size_t len = read_varint();
    if (remaining_.remove_prefix(len) != len)
      WIRE_DLOG_THROW(error::epee::not_enough_bytes, "not enough space for string");
    return {remaining_.data() - len, len};
  }

  void epee_reader::skip_fixed(const std::size_t count)
  {
    const std::size_t elem_size = min_wire_size(last_tag_);
    if (remaining_.size() / elem_size < count)
      WIRE_DLOG_THROW(error::epee::not_enough_bytes, count << " fixed size arithmetics of size " << elem_size);
    remaining_.remove_prefix(elem_size * count);
  }

  void epee_reader::skip_next()
  {
    skip_stack_.clear();
    skip_stack_.reserve(max_read_depth());
    do
    {
      switch (last_tag_)
      {
      default:
        skip_fixed(1); // skip a single fixed sized type
        break;
      case SERIALIZE_TYPE_ARRAY:
        skip_stack_.emplace_back(start_array(0), SERIALIZE_TYPE_ARRAY);
        break;
      case SERIALIZE_TYPE_OBJECT:
        skip_stack_.emplace_back(start_object(), SERIALIZE_TYPE_OBJECT);
        break;
      case SERIALIZE_TYPE_STRING:
        raw(error::schema::string);
        break;
      }

      // handle array/object stack
      if (!skip_stack_.empty())
      {
        if (skip_stack_.back().second == SERIALIZE_TYPE_ARRAY)
        {
          // skip fixed size types immediately
          if (last_tag_ != SERIALIZE_TYPE_ARRAY && last_tag_ != SERIALIZE_TYPE_OBJECT && last_tag_ != SERIALIZE_TYPE_STRING)
          {
            skip_fixed(skip_stack_.back().first);
            skip_stack_.back().first = 0;
          }

          if (!skip_stack_.back().first)
          {
            end_array();
            skip_stack_.pop_back();
          }
          else
            --skip_stack_.back().first;
        }
        else if (skip_stack_.back().second == SERIALIZE_TYPE_OBJECT)
        {
          if (!skip_stack_.back().first)
          {
            end_object();
            skip_stack_.pop_back();
          }
          else
          {
            read_name();
            last_tag_ = read_tag();
            --skip_stack_.back().first;
          }
        }
      }
    } while (!skip_stack_.empty());
  }

  epee_reader::epee_reader(const epee::span<const std::uint8_t> source)
    : reader(source),
      skip_stack_(),
      array_space_(source.size()),
      last_tag_(SERIALIZE_TYPE_OBJECT)
  {
    const auto sbh = read<epee::serialization::storage_block_header>();
    if (sbh.m_signature_a != SWAP32LE(PORTABLE_STORAGE_SIGNATUREA) ||
        sbh.m_signature_b != SWAP32LE(PORTABLE_STORAGE_SIGNATUREB))
      WIRE_DLOG_THROW_(error::epee::signature);
    if (sbh.m_ver != PORTABLE_STORAGE_FORMAT_VER)
      WIRE_DLOG_THROW_(error::epee::version);
  }

  epee_reader::~epee_reader() noexcept
  {}

  void epee_reader::check_complete() const
  {
    // do not consider extra bytes an error
    if (depth() || last_tag_ != SERIALIZE_TYPE_OBJECT)
      throw std::logic_error{"Invalid tree traversal"};
  }

  bool epee_reader::boolean()
  {
    if (last_tag_ != SERIALIZE_TYPE_BOOL)
      WIRE_DLOG_THROW_(error::schema::boolean);
    return read<bool>();
  }
  std::intmax_t epee_reader::integer()
  {
    switch (last_tag_)
    {
    case SERIALIZE_TYPE_INT64: return read<int64_t>();
    case SERIALIZE_TYPE_INT32: return read<int32_t>();
    case SERIALIZE_TYPE_INT16: return read<int16_t>();
    case SERIALIZE_TYPE_INT8:  return read<int8_t>();
    default:
      break;
    };
    WIRE_DLOG_THROW_(error::schema::integer);
  }

  std::uintmax_t epee_reader::unsigned_integer()
  {
    switch (last_tag_)
    {
    case SERIALIZE_TYPE_UINT64: return read<uint64_t>();
    case SERIALIZE_TYPE_UINT32: return read<uint32_t>();
    case SERIALIZE_TYPE_UINT16: return read<uint16_t>();
    case SERIALIZE_TYPE_UINT8:  return read<uint8_t>();
    default:
      break;
    };
    WIRE_DLOG_THROW_(error::schema::integer);
  }

  double epee_reader::real()
  {
    if (last_tag_ != SERIALIZE_TYPE_DUOBLE)
      WIRE_DLOG_THROW_(error::schema::number);
    return read<double>();
  }

  std::string epee_reader::string()
  {
    const auto value = raw(error::schema::string);
    return std::string{reinterpret_cast<const char*>(value.data()), value.size()};
  }

  epee::byte_slice epee_reader::binary()
  {
    // TODO update epee "glue" code to use byte_slice, so that this is a ref-count
    return epee::byte_slice{{raw(error::schema::binary)}};
  }

  void epee_reader::binary(epee::span<std::uint8_t> dest)
  {
    const auto value = raw(error::schema::binary);
    if (dest.size() != value.size())
      WIRE_DLOG_THROW(error::schema::fixed_binary, "of size" << dest.size() * 2 << " but got " << value.size());
    std::memcpy(dest.data(), value.data(), value.size());
  }

  std::size_t epee_reader::start_array(std::size_t min_element_size)
  {
    // also called from `skip_next`
    increment_depth();

    if (last_tag_ == SERIALIZE_TYPE_ARRAY)
      last_tag_ = read_tag();

    if (!(last_tag_ & SERIALIZE_FLAG_ARRAY))
      WIRE_DLOG_THROW_(error::schema::array);

    last_tag_ &= ~SERIALIZE_FLAG_ARRAY;

    const std::size_t count = read_varint();
    const std::size_t remaining = std::min(array_space_, remaining_.size());
    min_element_size = std::max(min_wire_size(last_tag_), min_element_size);
    if (min_element_size && ((remaining / min_element_size) < count))
      WIRE_DLOG_THROW(error::epee::not_enough_bytes, count << " array elements of at least " << min_element_size << " bytes each exceeds " << remaining << " remaining bytes");
    array_space_ = remaining - (count * min_element_size);
    return count;
  }

  bool epee_reader::is_array_end(const std::size_t count)
  {
    // also called from `skip_next`
    if (!count)
      last_tag_ = SERIALIZE_TYPE_ARRAY;
    return count == 0;
  }

  std::size_t epee_reader::start_object()
  {
    // also called from `skip_next`
    increment_depth();
    if (last_tag_ != SERIALIZE_TYPE_OBJECT)
      WIRE_DLOG_THROW_(error::schema::object);
    last_tag_ = 0;
    return read_varint();
  }

  bool epee_reader::key(const epee::span<const reader::key_map> map, std::size_t& state, std::size_t& index)
  {
    // state is the number of remaining fields on the wire
    while (state)
    {
      --state;

      const boost::string_ref name = read_name();
      last_tag_ = read_tag();
      for (std::size_t i = 0; i < map.size(); ++i)
      {
        if (name == map[i].name)
        {
          index = i;
          return true;
        }
      }
      skip_next();
    }
    last_tag_ = SERIALIZE_TYPE_OBJECT;
    return false;
  }
} // wire
