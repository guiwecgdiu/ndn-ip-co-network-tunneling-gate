/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2019  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "model/ndn-block-header.hpp"
#include "helper/ndn-stack-helper.hpp"

#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/encoding/block.hpp>

#include "ns3/ndnSIM/NFD/daemon/face/transport.hpp"
#include "ns3/packet.h"

#include "../tests-common.hpp"

namespace ns3 {
namespace ndn {

using ::ndn::operator "" _block;

BOOST_FIXTURE_TEST_SUITE(ModelNdnBlockHeader, CleanupFixture)

class EnablePacketPrintingFixture
{
public:
  EnablePacketPrintingFixture()
  {
    Packet::EnablePrinting();
  }
};

BOOST_GLOBAL_FIXTURE(EnablePacketPrintingFixture)
#if BOOST_VERSION >= 105900
;
#endif // BOOST_VERSION >= 105900


BOOST_AUTO_TEST_CASE(EncodePrintInterest)
{
  Interest interest("/prefix");
  interest.setNonce(10);
  interest.setCanBePrefix(true);
  lp::Packet lpPacket(interest.wireEncode());
  auto packet(lpPacket.wireEncode());
  BlockHeader header(packet);

  BOOST_CHECK_EQUAL(header.GetSerializedSize(), 20); // 20

  {
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    boost::test_tools::output_test_stream output;
    packet->Print(output);
    BOOST_CHECK(output.is_equal("ns3::ndn::Packet (Interest: /prefix?CanBePrefix&Nonce=0000000a)"));
  }
}

BOOST_AUTO_TEST_CASE(EncodePrintData)
{
  Data data("/other/prefix");
  data.setFreshnessPeriod(ndn::time::milliseconds(1000));
  data.setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(data);
  lp::Packet lpPacket(data.wireEncode());
  auto packet(lpPacket.wireEncode());
  BlockHeader header(packet);

  {
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    boost::test_tools::output_test_stream output;
    packet->Print(output);
    BOOST_CHECK(output.is_equal("ns3::ndn::Packet (Data: /other/prefix)"));
  }

  BOOST_TEST(data.getKeyLocator()->getName() == "/dummy/KEY/-%9C%28r%B8%AA%3B%60/NA/%FD%00%00%01%5E%DF%3C%C6%7C");
  BOOST_CHECK_EQUAL(header.GetSerializedSize(), 1365);
}

BOOST_AUTO_TEST_CASE(PrintLpPacket)
{
  Interest interest("/prefix");
  interest.setNonce(10);
  interest.setCanBePrefix(true);

  lp::Packet lpPacket;
  lpPacket.add<::ndn::lp::SequenceField>(0); // to make sure that the NDNLP header is added
  lpPacket.add<::ndn::lp::FragmentField>(std::make_pair(interest.wireEncode().begin(), interest.wireEncode().end()));

  {
    BlockHeader header(lpPacket.wireEncode());
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    boost::test_tools::output_test_stream output;
    packet->Print(output);
    BOOST_CHECK(output.is_equal("ns3::ndn::Packet (NDNLP(Interest: /prefix?CanBePrefix&Nonce=0000000a))"));
  }

  lpPacket.add<::ndn::lp::NackField>(::ndn::lp::NackHeader().setReason(::ndn::lp::NackReason::NO_ROUTE));

  {
    BlockHeader header(lpPacket.wireEncode());
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    boost::test_tools::output_test_stream output;
    packet->Print(output);
    BOOST_CHECK(output.is_equal("ns3::ndn::Packet (NDNLP(NACK(NoRoute) for Interest: /prefix?CanBePrefix&Nonce=0000000a))"));
  }

  lpPacket.remove<::ndn::lp::NackField>();
  lpPacket.add<::ndn::lp::FragIndexField>(0);
  lpPacket.add<::ndn::lp::FragCountField>(1);

  {
    BlockHeader header(lpPacket.wireEncode());
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    boost::test_tools::output_test_stream output;
    packet->Print(output);
    BOOST_CHECK(output.is_equal("ns3::ndn::Packet (NDNLP(Interest: /prefix?CanBePrefix&Nonce=0000000a))"));
  }

  lpPacket.set<::ndn::lp::FragCountField>(2);

  {
    BlockHeader header(lpPacket.wireEncode());
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    boost::test_tools::output_test_stream output;
    packet->Print(output);
    BOOST_CHECK(output.is_equal("ns3::ndn::Packet (NDNLP(fragment 1 out of 2))"));
  }

  Block unknown = "8808 0000 0000 0000 0000"_block;
  lpPacket.set<::ndn::lp::FragmentField>(std::make_pair(unknown.begin(), unknown.end()));
  lpPacket.remove<::ndn::lp::FragCountField>();
  lpPacket.remove<::ndn::lp::FragIndexField>();

  {
    BlockHeader header(lpPacket.wireEncode());
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    boost::test_tools::output_test_stream output;
    packet->Print(output);
    BOOST_CHECK(output.is_equal("ns3::ndn::Packet (NDNLP(Unrecognized))"));
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace ndn
} // namespace ns3
