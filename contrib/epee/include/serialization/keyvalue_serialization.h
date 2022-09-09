// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#pragma once

#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include "serialization/wire/fields.h"
#Include "serialization/wire/wrappers.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "serialization"

namespace epee
{
  template<typename F, typename... T, std::size_t... I>
  void unpack_object2(F& format, std::tuple<T...>&& fields, std::index_sequence<I...>)
  {
    /* Another ADL call below, delays function lookup until instatiation. This
       helps with forward declaring in headers. */
    wire_object(format, std::move(std::get<I>(fields))...);
  }
  template<typename F, typename... T>
  void unpack_object(F& format, std::tuple<T...>&& fields)
  {
    ::epee::unpack_object2(format, std::move(fields), std::make_index_sequence<sizeof...(T)>{});
  }
    
    
  /************************************************************************/
  /* Serialize map declarations                                           */
  /************************************************************************/
#define BEGIN_KV_SERIALIZE_MAP()                                        \
  template<typename R>                                                  \
  std::error_code from_bytes(const ::epee::span<const std::uint8_t> source) \
  { return ::wire_read::from_bytes<R>(source, *this); }                 \
                                                                        \
  std::error_code to_bytes(::epee::byte_stream& dest) const             \
  { return ::wire_write::to_bytes<W>(dest, *this); }                    \
                                                                        \
  template<class F>                                                     \
  void read_bytes(F& format)                                            \
  { ::epee::unpack_object(format, get_field_list(format, *this)); }     \
                                                                        \
  template<class F>                                                     \
  void write_bytes(F& format) const                                     \
  { ::epee::unpack_object(format, get_field_list(format, *this)); }     \
                                                                        \
  template<class F, class T>                                            \
  static auto get_field_list(F& format, T& self)                        \
  { return std::tuple_cat(

#define KV_SERIALIZE_N(varialble, val_name) \
  std::make_tuple(::wire::field(val_name, std::ref(self.varialble))),

#define KV_SERIALIZE_PARENT(type) \
  type::template get_field_list(format, self),

#define KV_SERIALIZE_OPT_N(variable, val_name, default_value) \
  std::make_tuple(::wire::optional_field(val_name, ::wire::defaulted(std::ref(self.variable), default_value))),

#define KV_SERIALIZE_VAL_POD_AS_BLOB_N(varialble, val_name) \
  std::make_tuple(::wire::field(val_name, ::wire::blob(std::ref(self.varialble)))),

#define KV_SERIALIZE_VAL_POD_AS_BLOB_OPT_N(varialble, val_name, default_value) \
  std::make_tuple(::wire::field(val_name, ::wire::defaulted(::wire::blob(std::ref(this_ref.varialble)), default_value))),

#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(varialble, val_name) \
  std::make_tuple(::wire::field(val_name, ::wire::array_as_blob(std::ref(self.varialble)))),

#define END_KV_SERIALIZE_MAP() std::make_tuple());}

#define KV_SERIALIZE(varialble)                           KV_SERIALIZE_N(varialble, #varialble)
#define KV_SERIALIZE_VAL_POD_AS_BLOB(varialble)           KV_SERIALIZE_VAL_POD_AS_BLOB_N(varialble, #varialble)
#define KV_SERIALIZE_VAL_POD_AS_BLOB_OPT(varialble, def)  KV_SERIALIZE_VAL_POD_AS_BLOB_OPT_N(varialble, #varialble, def)
#define KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(varialble)     KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(varialble, #varialble) //skip is_pod compile time check
#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB(varialble)     KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(varialble, #varialble)
#define KV_SERIALIZE_OPT(variable,default_value)          KV_SERIALIZE_OPT_N(variable, #variable, default_value)

}




