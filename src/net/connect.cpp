// Copyright (c) 2019-2020, The Monero Project
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

#include "connect.h"

#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <cstdint>
#include <memory>
#include <system_error>

#include "common/expect.h"
#include "net/error.h"
#include "net/parse.h"
#include "net/net_helper.h"
#include "net/resolve.h"
#include "net/socks.h"
#include "string_tools.h"

namespace net
{
namespace dnssec
{
    boost::unique_future<std::pair<boost::asio::ip::tcp::socket, std::vector<std::string>>>
    connector::operator()(const std::string& addr, const std::string& port, boost::asio::steady_timer& timeout, const bool fetch_tlsa) const
    {
        using future_type = std::pair<boost::asio::ip::tcp::socket, std::vector<std::string>>;

        if (!fetch_tlsa)
            return epee::net_utils::connect(addr, port, timeout);

        std::uint16_t porti = 0;
        if (!epee::string_tools::get_xtype_from_string(porti, port))
            return boost::make_exceptional_future<future_type>(std::system_error{net::error::invalid_port});

        // If IPv4/Ipv6 given, connect directly without DNS lookups
        {
            expect<boost::asio::ip::tcp::endpoint> endpoint = get_tcp_endpoint(addr, porti);
            if (endpoint)
                return epee::net_utils::connect(std::move(*endpoint), timeout);
            if (endpoint != net::error::unsupported_address)
                return boost::make_exceptional_future<future_type>(std::system_error{endpoint.error(), "Failed to parse " + addr});
        }

        expect<service_response> response = resolve_hostname(addr, port);
        if (!response)
            return boost::make_exceptional_future<future_type>(std::system_error{response.error(), "Failed to resolve " + addr});

        boost::system::error_code error{};
        auto ip_addr = boost::asio::ip::address::from_string(std::move(response->ip.front()), error);
        if (error)
            return boost::make_exceptional_future<future_type>(boost::system::system_error{error, "Invalid IP from DNS"});
        return epee::net_utils::connect({std::move(ip_addr), porti}, timeout, std::move(response->tlsa));
    }
} // dnssec
namespace socks
{
    boost::unique_future<std::pair<boost::asio::ip::tcp::socket, std::vector<std::string>>>
    connector::operator()(const std::string& remote_host, const std::string& remote_port, boost::asio::steady_timer& timeout, bool) const
    {
        struct future_socket
        {
            boost::promise<std::pair<boost::asio::ip::tcp::socket, std::vector<std::string>>> result_;

            void operator()(boost::system::error_code error, boost::asio::ip::tcp::socket&& socket)
            {
                if (error)
                    result_.set_exception(boost::system::system_error{error});
                else
                    result_.set_value({std::move(socket), {}});
            }
        };

        boost::unique_future<std::pair<boost::asio::ip::tcp::socket, std::vector<std::string>>> out{};
        {
            std::uint16_t port = 0;
            if (!epee::string_tools::get_xtype_from_string(port, remote_port))
                throw std::system_error{net::error::invalid_port, "Remote port for socks proxy"};

            bool is_set = false;
            std::uint32_t ip_address = 0;
            boost::promise<std::pair<boost::asio::ip::tcp::socket, std::vector<std::string>>> result{};
            out = result.get_future();
            const auto proxy = net::socks::make_connect_client(
                boost::asio::ip::tcp::socket{GET_IO_SERVICE(timeout)}, net::socks::version::v4a, future_socket{std::move(result)}
            );

            if (epee::string_tools::get_ip_int32_from_string(ip_address, remote_host))
                is_set = proxy->set_connect_command(epee::net_utils::ipv4_network_address{ip_address, port});
            else
                is_set = proxy->set_connect_command(remote_host, port);

            if (!is_set || !net::socks::client::connect_and_send(proxy, proxy_address))
                throw std::system_error{net::error::invalid_host, "Address for socks proxy"};

            timeout.async_wait(net::socks::client::async_close{std::move(proxy)});
        }

        return out;
    }
} // socks
} // net
