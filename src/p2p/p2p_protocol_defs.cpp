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

#include "p2p/p2p_protocol_defs.h"

#include <type_traits>

#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "net/serialization.h"
#include "serialization/wire/array.h"
#include "serialization/wire/defaulted.h"
#include "serialization/wire/epee.h"
#include "serialization/wire/traits.h"

namespace wire
{
  template<>
  struct is_blob<boost::uuids::uuid>
    : std::true_type
  {};
}

namespace nodetool
{
  namespace
  {
    using peerlist_max = wire::max_element_count<P2P_MAX_PEERS_IN_HANDSHAKE>;

    template<typename F, typename T>
    void peerlist_entry_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(adr),
        WIRE_FIELD(id),
        WIRE_FIELD_DEFAULTED(last_seen, unsigned(0)),
        WIRE_FIELD_DEFAULTED(pruning_seed, unsigned(0)),
        WIRE_FIELD_DEFAULTED(rpc_port, unsigned(0)),
        WIRE_FIELD_DEFAULTED(rpc_credits_per_hash, unsigned(0))
      );
    }

    template<typename F, typename T>
    void anchor_peerlist_entry_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(adr), WIRE_FIELD(id), WIRE_FIELD(first_seen));
    }

    template<typename F, typename T>
    void connection_entry_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(adr), WIRE_FIELD(id), WIRE_FIELD(is_income));
    }

    template<typename F, typename T>
    void network_config_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(max_out_connection_count),
        WIRE_FIELD(max_in_connection_count),
        WIRE_FIELD(connection_timeout),
        WIRE_FIELD(ping_connection_timeout),
        WIRE_FIELD(handshake_interval),
        WIRE_FIELD(packet_max_size),
        WIRE_FIELD(config_id),
        WIRE_FIELD(send_peerlist_sz)
      );
    }

    template<typename F, typename T>
    void basic_node_data_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(network_id),
        WIRE_FIELD(my_port),
        WIRE_FIELD_DEFAULTED(rpc_port, unsigned(0)),
        WIRE_FIELD_DEFAULTED(rpc_credits_per_hash, unsigned(0)),
        WIRE_FIELD_DEFAULTED(peer_id, unsigned(0)),
        WIRE_FIELD_DEFAULTED(support_flags, unsigned(0))
      );
    }

    template<typename F, typename T>
    void handshake_request_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(node_data), WIRE_FIELD(payload_data));
    }

    template<typename F, typename T>
    void handshake_response_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(node_data),
        WIRE_FIELD(payload_data),
        WIRE_FIELD_ARRAY(local_peerlist_new, peerlist_max)
      );
    }

    template<typename F, typename T>
    void timed_sync_request_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(payload_data));
    }

    template<typename F, typename T>
    void timed_sync_response_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(payload_data),
        WIRE_FIELD_ARRAY(local_peerlist_new, peerlist_max)
      );
    }

    template<typename F, typename T>
    void ping_request_map(F& format, T& self)
    {
      wire::object(format);
    }

    template<typename F, typename T>
    void ping_response_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(status), WIRE_FIELD(peer_id));
    }

    template<typename F, typename T>
    void support_flags_request_map(F& format, T& self)
    {
      wire::object(format);
    }

    template<typename F, typename T>
    void support_flags_response_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(support_flags));
    }
  } // anonymous

  WIRE_EPEE_DEFINE_OBJECT(peerlist_entry, peerlist_entry_map);
  WIRE_EPEE_DEFINE_OBJECT(anchor_peerlist_entry, anchor_peerlist_entry_map);
  WIRE_EPEE_DEFINE_OBJECT(connection_entry, connection_entry_map);
  WIRE_EPEE_DEFINE_OBJECT(network_config, network_config_map);
  WIRE_EPEE_DEFINE_OBJECT(basic_node_data, basic_node_data_map);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::request, handshake_request_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::request);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::response, handshake_response_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::response);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::request, timed_sync_request_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::request);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::response, timed_sync_response_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::response);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_PING::request, ping_request_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_PING::request);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_PING::response, ping_response_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_PING::response);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_REQUEST_SUPPORT_FLAGS::request, support_flags_request_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_REQUEST_SUPPORT_FLAGS::request);
  WIRE_EPEE_DEFINE_OBJECT(COMMAND_REQUEST_SUPPORT_FLAGS::response, support_flags_response_map);
  WIRE_EPEE_DEFINE_CONVERSION(COMMAND_REQUEST_SUPPORT_FLAGS::response);
} // nodetool
