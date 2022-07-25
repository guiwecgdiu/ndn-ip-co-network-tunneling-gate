/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2021 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "ndn-cxx/net/dns.hpp"

#include "tests/boost-test.hpp"
#include "tests/unit/net/network-configuration-detector.hpp"

#include <boost/asio/io_service.hpp>

namespace ndn {
namespace dns {
namespace tests {

using boost::asio::ip::address_v4;
using boost::asio::ip::address_v6;

class DnsFixture
{
public:
  void
  onSuccess(const IpAddress& resolvedAddress,
            const IpAddress& expectedAddress,
            bool isValid,
            bool shouldCheckAddress = false)
  {
    ++m_nSuccesses;

    if (!isValid) {
      BOOST_ERROR("Resolved to " + resolvedAddress.to_string() + ", but should have failed");
    }

    BOOST_CHECK_EQUAL(resolvedAddress.is_v4(), expectedAddress.is_v4());

    // checking address is not deterministic and should be enabled only
    // if only one IP address will be returned by resolution
    if (shouldCheckAddress) {
      BOOST_CHECK_EQUAL(resolvedAddress, expectedAddress);
    }
  }

  void
  onFailure(bool shouldFail)
  {
    ++m_nFailures;
    BOOST_CHECK_MESSAGE(shouldFail, "Resolution should not have failed");
  }

protected:
  int m_nFailures = 0;
  int m_nSuccesses = 0;
  boost::asio::io_service m_ioService;
};

BOOST_AUTO_TEST_SUITE(Net)
BOOST_FIXTURE_TEST_SUITE(TestDns, DnsFixture)

BOOST_AUTO_TEST_CASE(Asynchronous)
{
  SKIP_IF_IP_UNAVAILABLE();

  asyncResolve("nothost.nothost.nothost.arpa",
               std::bind(&DnsFixture::onSuccess, this, _1, IpAddress(address_v4()), false, false),
               [this] (auto&&...) { onFailure(true); },
               m_ioService); // should fail

  m_ioService.run();
  BOOST_CHECK_EQUAL(m_nFailures, 1);
  BOOST_CHECK_EQUAL(m_nSuccesses, 0);
}

BOOST_AUTO_TEST_CASE(AsynchronousV4)
{
  SKIP_IF_IPV4_UNAVAILABLE();

  asyncResolve("192.0.2.1",
               std::bind(&DnsFixture::onSuccess, this, _1,
                         IpAddress(address_v4::from_string("192.0.2.1")), true, true),
               [this] (auto&&...) { onFailure(false); },
               m_ioService);

  m_ioService.run();
  BOOST_CHECK_EQUAL(m_nFailures, 0);
  BOOST_CHECK_EQUAL(m_nSuccesses, 1);
}

BOOST_AUTO_TEST_CASE(AsynchronousV6)
{
  SKIP_IF_IPV6_UNAVAILABLE();

  asyncResolve("ipv6.google.com", // only IPv6 address should be available
               std::bind(&DnsFixture::onSuccess, this, _1, IpAddress(address_v6()), true, false),
               [this] (auto&&...) { onFailure(false); },
               m_ioService);

  asyncResolve("2001:db8:3f9:0:3025:ccc5:eeeb:86d3",
               std::bind(&DnsFixture::onSuccess, this, _1,
                         IpAddress(address_v6::from_string("2001:db8:3f9:0:3025:ccc5:eeeb:86d3")),
                         true, true),
               [this] (auto&&...) { onFailure(false); },
               m_ioService);

  m_ioService.run();
  BOOST_CHECK_EQUAL(m_nFailures, 0);
  BOOST_CHECK_EQUAL(m_nSuccesses, 2);
}

BOOST_AUTO_TEST_CASE(AsynchronousV4AndV6)
{
  SKIP_IF_IPV4_UNAVAILABLE();
  SKIP_IF_IPV6_UNAVAILABLE();

  asyncResolve("www.named-data.net",
               std::bind(&DnsFixture::onSuccess, this, _1, IpAddress(address_v4()), true, false),
               [this] (auto&&...) { onFailure(false); },
               m_ioService, Ipv4Only());

  asyncResolve("a.root-servers.net",
               std::bind(&DnsFixture::onSuccess, this, _1, IpAddress(address_v4()), true, false),
               [this] (auto&&...) { onFailure(false); },
               m_ioService, Ipv4Only()); // request IPv4 address

  asyncResolve("a.root-servers.net",
               std::bind(&DnsFixture::onSuccess, this, _1, IpAddress(address_v6()), true, false),
               [this] (auto&&...) { onFailure(false); },
               m_ioService, Ipv6Only()); // request IPv6 address

  asyncResolve("ipv6.google.com", // only IPv6 address should be available
               std::bind(&DnsFixture::onSuccess, this, _1, IpAddress(address_v6()), true, false),
               [this] (auto&&...) { onFailure(false); },
               m_ioService, Ipv6Only());

  asyncResolve("ipv6.google.com", // only IPv6 address should be available
               std::bind(&DnsFixture::onSuccess, this, _1, IpAddress(address_v6()), false, false),
               [this] (auto&&...) { onFailure(true); },
               m_ioService, Ipv4Only()); // should fail

  m_ioService.run();
  BOOST_CHECK_EQUAL(m_nFailures, 1);
  BOOST_CHECK_EQUAL(m_nSuccesses, 4);
}

BOOST_AUTO_TEST_CASE(Synchronous)
{
  SKIP_IF_IP_UNAVAILABLE();

  IpAddress address = syncResolve("www.named-data.net", m_ioService);
  BOOST_CHECK(address.is_v4() || address.is_v6());
}

BOOST_AUTO_TEST_SUITE_END() // TestDns
BOOST_AUTO_TEST_SUITE_END() // Net

} // namespace tests
} // namespace dns
} // namespace ndn
