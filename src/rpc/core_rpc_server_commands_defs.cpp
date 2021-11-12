// Copyright (c) 2021, The Monero Project
//
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

#include "core_rpc_server_commands_defs.h"

#include "cryptonote_config.h"
#include "serialization/wire/array.h"
#include "serialization/wire/array_blob.h"
#include "serialization/wire/defaulted.h"
#include "serialization/wire/epee.h"

#define RPC_ACCESS_REQUEST_BASE()               \
  WIRE_FIELD(client)

#define RPC_RESPONSE_BASE()                     \
  WIRE_FIELD(status), WIRE_FIELD(untrusted)

#define RPC_ACCESS_RESPONSE_BASE()                                      \
  RPC_RESPONSE_BASE(), WIRE_FIELD(credits), WIRE_FIELD(top_hash)


namespace cryptonote
{
  namespace
  {
    template<typename F>
    constexpr std::size_t min_uint64(const F&) noexcept
    {
      static_assert(std::is_same<F, wire::epee_reader>::value || std::is_same<F, wire::epee_writer>::value, "incorrect for format type");
      return sizeof(std::uint64_t);
    }

    std::vector<std::uint64_t> decompress_integer_array(const std::string &s)
    {
      std::vector<std::uint64_t> v;
      v.reserve(s.size());
      int read = 0;
      const std::string::const_iterator end = s.end();
      for (std::string::const_iterator i = s.begin(); i != end; std::advance(i, read))
      {
        v.emplace_back();
        read = tools::read_varint(std::string::const_iterator(i), s.end(), v.back());
        CHECK_AND_ASSERT_THROW_MES(read > 0 && read <= 256, "Error decompressing data");
      }
      return v;
    }
    std::string compress_integer_array(const std::vector<std::uint64_t> &v)
    {
      std::string s;
      s.resize(v.size() * (sizeof(T) * 8 / 7 + 1));
      char *ptr = (char*)s.data();
      for (const std::uint64_t t: v)
        tools::write_varint(ptr, t);
      s.resize(ptr - s.data());
      return s;
    }

    template<typename F, typename T>
    void get_blocks_request_map(F& format, T& self)
    {
      wire::object(format,
        RPC_ACCESS_REQUEST_BASE(),
        WIRE_FIELD_ARRAY_AS_BLOB(block_ids),
        WIRE_FIELD(start_height),
        WIRE_FIELD(prune),
        WIRE_FIELD_DEFAULTED(no_miner_tx, false)
      );
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_BLOCKS_FAST::request, get_blocks_request_map);

    template<typename F, typename T>
    void tx_output_indices_map(F& format, T& self)
    {
      using indices_min = wire::min_element_size<min_uint64(format)>;
      wire::object(format, WIRE_FIELD_ARRAY(indices, indices_min));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_BLOCKS_FAST::tx_output_indices, tx_output_indices_map);

    template<typename F, typename T>
    void block_output_indices_map(F& format, T& self)
    {
      using max_txes = wire::max_element_count<COMMAND_RPC_GET_BLOCKS_FAST_MAX_TX_COUNT>;
      wire::object(format, WIRE_FIELD_ARRAY(indices, max_txes));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices, block_output_indices_map);

    template<typename F, typename T>
    void get_blocks_response_map(F& format, T& self)
    {
      using max_blocks = wire::max_element_count<COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT>;
      wire::object(format,
        RPC_ACCESS_RESPONSE_BASE(),
        WIRE_FIELD_ARRAY(blocks, max_blocks),
        WIRE_FIELD(start_height),
        WIRE_FIELD(currrent_height),
        WIRE_FIELD_ARRAY(output_indices, max_blocks)
      );
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_BLOCKS_FAST::response, get_blocks_response_map);

    template<typename F, typename T>
    void blocks_by_height_request_map(F& format, T& self)
    {
      using height_min = wire::min_element_size<min_uint64(format)>;
      wire::object(format, RPC_ACCESS_REQUEST_BASE(), WIRE_FIELD_ARRAY(height, height_min));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request, blocks_by_height_request_map);

    template<typename F, typename T>
    void blocks_by_height_response_map(F& format, T& self)
    {
      wire::object(format, RPC_ACCESS_RESPONSE_BASE(), WIRE_FIELD(blocks));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response, blocks_by_height_response_map);

    template<typename F, typename T>
    void get_hashes_request_map(F& format, T& self)
    {
      wire::object(format, RPC_ACCESS_REQUEST_BASE(), WIRE_FIELD_ARRAY_AS_BLOB(block_ids), WIRE_FIELD(start_height));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_HASHES_FAST::request, get_hashes_request_map);

    template<typename F, typename T>
    void get_hashes_response_map(F& format, T& self)
    {
      wire::object(format,
        RPC_ACCESS_RESPONSE_BASE(),
        WIRE_FIELD_ARRAY_AS_BLOB(m_block_ids),
        WIRE_FIELD(start_height),
        WIRE_FIELD(current_height)
      );
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_HASHES_FAST::response, get_hashes_response_map);

    template<typename F, typename T>
    void get_output_indexes_request_map(F& format, T& self)
    {
      wire::object(format, RPC_ACCESS_REQUEST_BASE(), WIRE_FIELD(txid));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request, get_output_indexes_request_map);

    template<typename F, typename T>
    void get_output_indexes_response_map(F& format, T& self)
    {
      using index_min = wire::min_element_size<min_uint64(format)>;
      wire::object(format, RPC_ACCESS_RESPONSE_BASE(), WIRE_FIELD_ARRAY(o_indexes, index_min));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response, get_output_indexes_response_map);

    template<typename F, typename T>
    void get_outputs_out_map(F& format, T& self)
    {
      // see get_outputs_request_map if changing fields
      wire::object(format, WIRE_FIELD(amount), WIRE_FIELD(index));
    }
    WIRE_EPEE_DEFINE_OBJECT(get_outputs_out, get_outputs_out_map);

    template<typename F, typename T>
    void get_outputs_request_map(F& format, T& self)
    {
      using outputs_out_min = wire::min_element_size<min_uint64(format) * 2>;
      wire::object(format, RPC_ACCESS_REQUEST_BASE(), WIRE_FIELD_ARRAY(outputs, outputs_out_min), WIRE_FIELD_DEFAULTED(get_txid, true));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_OUTPUTS_BIN::request, get_outputs_request_map);

    template<typename F, typename T>
    void outkey_map(F& format, T& self)
    {
      // see get_outputs_response_map if changing fields
      wire::object(format, WIRE_FIELD(key), WIRE_FIELD(mask), WIRE_FIELD(unlocked), WIRE_FIELD(height), WIRE_FIELD(txid));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_OUTPUTS_BIN::outkey, outkey_map);

    template<typename F, typename T>
    void get_outputs_response_map(F& format, T& self)
    {
      static constexpr std::size_t outkey_min_value =
        sizeof(crypto::public_key) + sizeof(rct::key) + min_uint64(format) + sizeof(crypto::hash);
      using outkey_min = wire::min_element_size<min_outkey_value>;
      wire::object(format, RPC_ACCESS_RESPONSE_BASE(), WIRE_FIELD_ARRAY(outs, min_outkey));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_OUTPUTS_BIN::response, get_outputs_response_map);

    template<typename F, typename T>
    void output_distribution_request_map(F& format, T& self)
    {
      using amounts_min = wire::min_element_size<min_uint64(format)>;
      wire::object(format,
        WIRE_FIELD_ARRAY(amounts, amounts_min),
        WIRE_FIELD_DEFAULTED(from_height, 0),
        WIRE_FIELD_DEFAULTED(to_height, 0),
        WIRE_FIELD_DEFAULTED(cumulative, false),
        WIRE_FIELD_DEFAULTED(binary, true),
        WIRE_FIELD_DEFAULTED(compress, false)
      );
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request, output_distribution_request_map);

    enum class is_blob { false_ = 0, true_ };
    void read_bytes(wire::epee_reader& source, std::pair<std::vector<std::uint64_t>, is_blob>& dest)
    {
      if (source.last_tag() == SERIALIZE_TYPE_STRING)
      {
        wire::read_as_blob(source, dest.first);
        dest.second = is_blob::true_;
      }
      else
      {
        using element_min = wire::min_element_size<min_uint64(source)>;
        wire_read::array(source, dest.first, element_min);
        dest.second = is_blob::false_;
      }
    }
    void write_bytes(wire::epee_writer& dest, const std::pair<const std::vector<std::uint64_t>&, is_blob> source)
    {
      if (source.second == is_blob::true_)
        wire::write_as_blob(dest, source.first);
      else
        wire_write::array(dest, source.first);
    }

    template<typename F, typename T, typename U>
    void output_distribution_map(F& format, T& self, boost::optional<std::string>& compressed, U& binary)
    {
      wire::object(format,
        WIRE_FIELD(amount),
        wire::field("start_height", std::ref(self.data.start_height)),
        WIRE_FIELD(binary),
        WIRE_FIELD(compress),
        wire::optional_field("compressed_data", std::ref(compressed)),
        wire::optional_field("distribution", std::ref(binary)),
        wire::field("base", std::ref(self.data.base))
      );
    }
    void read_bytes(wire::epee_reader& source, COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::distribution& dest)
    {
      boost::optional<std::string> compressed;
      boost::optional<std::pair<std::vector<std::uint64_t>, is_blob>> binary;
      output_distribution_map(source, dest, compressed, binary);

      if (compressed && dest.binary && dest.compress)
        self.data.distribution = decompress_integer_array(*compressed);
      else if (binary && bool(binary->second) == dest.binary && !dest.compress)
        self.data.distribution = std::move(binary->first);
      else
        WIRE_DLOG_THROW(wire::schema::array, "distribution array sent incorrectly");
    }
    void write_bytes(wire::epee_writer& dest, const COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::distribution& source)
    {
      boost::optional<std::string> compressed;
      boost::optional<std::pair<const std::vector<std::uint64_t>&, is_blob>> binary;
      if (source.binary && source.compress)
        compressed = compress_integer_array(source.data.distribution);
      else
        binary.emplace(source.data.distribution, is_blob(source.binary));

      output_distribution_map(dest, source, compressed, binary);
    }

    template<typename F, typename T>
    void output_distribution_response_map(F& format, T& self)
    {
      using distributions_max = wire::max_element_count<>;
      wire::object(format, WIRE_FIELD_ARRAY(distributions, distributions_max));
    }
    WIRE_EPEE_DEFINE_OBJECT(COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response, output_distribution_response_map);
  } // anonymous

  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_BLOCKS_FAST::request);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_BLOCKS_FAST::response);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request)
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_HASHES_FAST::request);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_HASHES_FAST::response);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_TX_GLOBAL_OUTPUT_INDEXES::request);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_TX_GLOBAL_OUTPUT_INDEXES::response);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_OUTPUTS_BIN::request);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_OUTPUTS_BIN::response);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response);
} // cryptonote
