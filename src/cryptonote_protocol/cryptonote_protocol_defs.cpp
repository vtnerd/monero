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

#include "cryptonote_protocol_defs.h"

#include <boost/range/adaptor/transformed.hpp>
#include <tuple>

#include "serialization/wire/array.h"
#include "serialization/wire/array_blob.h"
#include "serialization/wire/defaulted.h"
#include "serialization/wire/epee.h"
#include "serialization/wire/traits.h"
#include "storages/portable_storage_base.h"

namespace cryptonote
{
  namespace
  {
    using tx_blob_min = wire::min_element_size<41>;

    template<typename F, typename T>
    void tx_blob_entry_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(blob), WIRE_FIELD(prunable_hash));
    }

    //! is_pruned ensures unique read/write functions
    enum class is_pruned { false_ = 0, true_ };
    void read_bytes(wire::epee_reader& source, std::tuple<std::vector<tx_blob_entry>&, is_pruned&> dest)
    {
      // read function for `tx_blob_entry` detects whether pruned (object) or !pruned (string)
      wire_read::array(source, std::get<0>(dest), tx_blob_min{});
      std::get<1>(dest) = is_pruned(source.last_tag() == SERIALIZE_TYPE_OBJECT);
    }

    void write_bytes(wire::epee_writer& dest, const std::tuple<const std::vector<tx_blob_entry>&, is_pruned> source)
    {
      const auto get_blob = [] (const tx_blob_entry& e) -> const std::string& { return e.blob; };
      if (bool(std::get<1>(source))) // pruned -> write array of `tx_blob_entry` objects
        wire_write::array(dest, std::get<0>(source));
      else // !pruned -> write array of string blobs
        wire_write::array(dest, boost::adaptors::transform(std::get<0>(source), get_blob));
    }

    template<typename F, typename T>
    void block_complete_entry_map(F& format, T& self)
    {
      is_pruned result = is_pruned(self.pruned);
      wire::object(format,
        WIRE_FIELD_DEFAULTED(pruned, false),
        WIRE_FIELD(block),
        WIRE_FIELD_DEFAULTED(block_weight, unsigned(0)),
        wire::field("txs", std::tie(self.txs, result))
      );
      if (bool(result) != self.pruned)
        WIRE_DLOG_THROW(wire::error::schema::object, "Schema mismatch with pruned flag set to " << self.pruned);
    }

    template<typename F, typename T>
    void new_block_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(b), WIRE_FIELD(current_blockchain_height));
    }

    template<typename F, typename T>
    void new_transactions_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD_ARRAY(txs, tx_blob_min),
        WIRE_FIELD(_),
        WIRE_FIELD_DEFAULTED(dandelionpp_fluff, true) // backwards compatible mode is fluff
      );
    }

    template<typename F, typename T>
    void request_get_objects_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD_ARRAY_AS_BLOB(blocks), WIRE_FIELD_DEFAULTED(prune, false));
    }

    template<typename F, typename T>
    void response_get_objects_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD_ARRAY(blocks, block_blob_min),
        WIRE_FIELD_ARRAY_AS_BLOB(missed_ids),
        WIRE_FIELD(current_blockchain_height)
      );
    }

    template<typename F, typename T>
    void core_sync_data_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(current_height),
        WIRE_FIELD(cumulative_difficulty),
        WIRE_FIELD_DEFAULTED(cumulative_difficulty_top64, unsigned(0)),
        WIRE_FIELD(top_id),
        WIRE_FIELD_DEFAULTED(top_version, unsigned(0)),
        WIRE_FIELD_DEFAULTED(pruning_seed, unsigned(0))
      );
    }

    template<typename F, typename T>
    void request_chain_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD_ARRAY_AS_BLOB(block_ids), WIRE_FIELD_DEFAULTED(prune, false));
    }

    template<typename F, typename T>
    void response_chain_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(start_height),
        WIRE_FIELD(total_height),
        WIRE_FIELD(cumulative_difficulty),
        WIRE_FIELD_DEFAULTED(cumulative_difficulty_top64, unsigned(0)),
        WIRE_FIELD_ARRAY_AS_BLOB(m_block_ids),
        WIRE_FIELD_ARRAY_AS_BLOB(m_block_weights),
        WIRE_FIELD(first_block)
      );
    }

    template<typename F, typename T>
    void new_fluffy_blob_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(b), WIRE_FIELD(current_blockchain_height));
    }

    template<typename F, typename T>
    void fluffy_missing_tx_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(block_hash),
        WIRE_FIELD(current_blockchain_height),
        WIRE_FIELD_ARRAY_AS_BLOB(missing_tx_indices)
      );
    }

    template<typename F, typename T>
    void txpool_complement_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD_ARRAY_AS_BLOB(hashes));
    }
  } // anonymous

  void read_bytes(wire::epee_reader& source, tx_blob_entry& dest)
  {
    if (source.last_tag() == SERIALIZE_TYPE_STRING)
      wire_read::bytes(source, dest.blob);
    else
      tx_blob_entry_map(source, dest);
  }
  void write_bytes(wire::epee_writer& dest, const tx_blob_entry& source)
  {
    tx_blob_entry_map(dest, source);
  }
  WIRE_EPEE_DEFINE_OBJECT(block_complete_entry, block_complete_entry_map);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_NEW_BLOCK::request, new_block_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_NEW_BLOCK::request);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_NEW_TRANSACTIONS::request, new_transactions_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_NEW_TRANSACTIONS::request)
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_REQUEST_GET_OBJECTS::request, request_get_objects_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_REQUEST_GET_OBJECTS::request);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_RESPONSE_GET_OBJECTS::request, response_get_objects_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_RESPONSE_GET_OBJECTS::request);
  WIRE_EPEE_DEFINE_OBJECT(CORE_SYNC_DATA, core_sync_data_map);
  WIRE_EPEE_DEFINE_CONVERSION(CORE_SYNC_DATA);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_REQUEST_CHAIN::request, request_chain_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_REQUEST_CHAIN::request);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_RESPONSE_CHAIN_ENTRY::request, response_chain_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_RESPONSE_CHAIN_ENTRY::request);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_NEW_FLUFFY_BLOCK::request, new_fluffy_blob_map);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_REQUEST_FLUFFY_MISSING_TX::request, fluffy_missing_tx_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_NEW_FLUFFY_BLOCK::request);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_REQUEST_FLUFFY_MISSING_TX::request);
  WIRE_EPEE_DEFINE_OBJECT(NOTIFY_GET_TXPOOL_COMPLEMENT::request, txpool_complement_map);
  WIRE_EPEE_DEFINE_CONVERSION(NOTIFY_GET_TXPOOL_COMPLEMENT::request);
} // cryptonote
