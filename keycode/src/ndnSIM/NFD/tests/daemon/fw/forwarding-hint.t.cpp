/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fw/best-route-strategy.hpp"

#include "tests/daemon/global-io-fixture.hpp"
#include "topology-tester.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

#define CHECK_FH_EQUAL(actual, ...) \
  do { \
    std::vector<Name> expected({__VA_ARGS__}); \
    BOOST_CHECK_EQUAL_COLLECTIONS( \
      (actual).getForwardingHint().begin(), (actual).getForwardingHint().end(), \
      expected.begin(), expected.end()); \
  } while (false)

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_AUTO_TEST_SUITE(TestForwardingHint)

/**
 *      /arizona/cs/avenir
 *              |
 *              v
 *      /arizona/cs/hobo
 *      |              |
 *      v              v
 *  /telia/terabits    /ucsd/caida/click
 *        |                 |
 *        v                 v
 *  /net/ndnsim        /ucla/cs/spurs
 *   (serverP)              |
 *                          v
 *                     /net/ndnsim
 *                      (serverQ)
 */
class NdnsimTeliaUclaTopologyFixture : public GlobalIoTimeFixture
{
public:
  NdnsimTeliaUclaTopologyFixture()
  {
    topo.enablePcap();

    nodeA = topo.addForwarder("avenir");
    nodeH = topo.addForwarder("hobo");
    nodeT = topo.addForwarder("terabits");
    nodeP = topo.addForwarder("serverP");
    nodeC = topo.addForwarder("click");
    nodeS = topo.addForwarder("spurs");
    nodeQ = topo.addForwarder("serverQ");
    for (auto node : {nodeA, nodeH, nodeT, nodeP, nodeC, nodeS, nodeQ}) {
      topo.setStrategy<BestRouteStrategy>(node);
    }

    topo.getForwarder(nodeH).getNetworkRegionTable().insert("/arizona/cs/hobo");
    topo.getForwarder(nodeT).getNetworkRegionTable().insert("/telia/terabits/router");
    topo.getForwarder(nodeC).getNetworkRegionTable().insert("/ucsd/caida/click");
    topo.getForwarder(nodeS).getNetworkRegionTable().insert("/ucla/cs/spurs");
    // NetworkRegionTable configuration is unnecessary on end hosts

    linkAH = topo.addLink("AH", 10_ms, {nodeA, nodeH});
    linkHT = topo.addLink("HT", 10_ms, {nodeH, nodeT});
    linkTP = topo.addLink("TP", 10_ms, {nodeT, nodeP});
    linkHC = topo.addLink("HC", 10_ms, {nodeH, nodeC});
    linkCS = topo.addLink("CS", 10_ms, {nodeC, nodeS});
    linkSQ = topo.addLink("SQ", 10_ms, {nodeS, nodeQ});
    consumerA = topo.addAppFace("avenir", nodeA);
    producerP = topo.addAppFace("ndnsimP", nodeP, "/net/ndnsim");
    producerQ = topo.addAppFace("ndnsimQ", nodeQ, "/net/ndnsim");

    topo.addEchoProducer(producerP->getClientFace());
    topo.addEchoProducer(producerQ->getClientFace());

    topo.registerPrefix(nodeA, linkAH->getFace(nodeA), "/", 10);
    topo.registerPrefix(nodeH, linkHT->getFace(nodeH), "/telia", 10);
    topo.registerPrefix(nodeT, linkTP->getFace(nodeT), "/net/ndnsim", 10);
    topo.registerPrefix(nodeH, linkHC->getFace(nodeH), "/ucla", 20);
    topo.registerPrefix(nodeC, linkCS->getFace(nodeC), "/ucla", 10);
    topo.registerPrefix(nodeS, linkSQ->getFace(nodeS), "/net/ndnsim", 10);
  }

  /** \brief express an Interest with Link object from consumerA
   */
  void
  consumerExpressInterest(int seq)
  {
    auto interest = makeInterest(Name("/net/ndnsim").appendNumber(seq));
    interest->setForwardingHint({nameTelia, nameUcla});
    consumerA->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  }

public:
  TopologyTester topo;
  TopologyNode nodeA, nodeH, nodeT, nodeP, nodeC, nodeS, nodeQ;
  shared_ptr<TopologyLink> linkAH, linkHT, linkTP, linkHC, linkCS, linkSQ;
  shared_ptr<TopologyAppLink> consumerA, producerP, producerQ;

  Name nameTelia = "/telia/terabits";
  Name nameUcla = "/ucla/cs";
};

BOOST_FIXTURE_TEST_SUITE(NdnsimTeliaUclaTopology, NdnsimTeliaUclaTopologyFixture)

BOOST_AUTO_TEST_CASE(FetchTelia)
{
  this->consumerExpressInterest(1);
  this->advanceClocks(11_ms, 20);

  // A forwards Interest according to default route, no change to forwarding hint
  BOOST_CHECK_EQUAL(linkAH->getFace(nodeA).getCounters().nOutInterests, 1);
  const Interest& interestAH = topo.getPcap(linkAH->getFace(nodeA)).sentInterests.at(0);
  CHECK_FH_EQUAL(interestAH, nameTelia, nameUcla);

  // H prefers T, no change to forwarding hint
  BOOST_CHECK_EQUAL(linkHT->getFace(nodeH).getCounters().nOutInterests, 1);
  const Interest& interestHT = topo.getPcap(linkHT->getFace(nodeH)).sentInterests.at(0);
  CHECK_FH_EQUAL(interestHT, nameTelia, nameUcla);

  // T forwards to P, forwarding hint stripped when Interest reaches producer region
  BOOST_CHECK_EQUAL(linkTP->getFace(nodeT).getCounters().nOutInterests, 1);
  const Interest& interestTP = topo.getPcap(linkTP->getFace(nodeT)).sentInterests.at(0);
  BOOST_CHECK(interestTP.getForwardingHint().empty());

  // Data is served by P and reaches A
  BOOST_CHECK_EQUAL(producerP->getForwarderFace().getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(consumerA->getForwarderFace().getCounters().nOutData, 1);
}

BOOST_AUTO_TEST_CASE(FetchUcla)
{
  // disconnect H-T and delete FIB entry
  linkHT->fail();
  topo.getForwarder(nodeH).getFib().erase("/telia");

  this->consumerExpressInterest(1);
  this->advanceClocks(11_ms, 20);

  // A forwards Interest according to default route, no change to forwarding hint
  BOOST_CHECK_EQUAL(linkAH->getFace(nodeA).getCounters().nOutInterests, 1);
  const Interest& interestAH = topo.getPcap(linkAH->getFace(nodeA)).sentInterests.at(0);
  CHECK_FH_EQUAL(interestAH, nameTelia, nameUcla);

  // H forwards to C, no change to forwarding hint
  BOOST_CHECK_EQUAL(linkHC->getFace(nodeH).getCounters().nOutInterests, 1);
  const Interest& interestHC = topo.getPcap(linkHC->getFace(nodeH)).sentInterests.at(0);
  CHECK_FH_EQUAL(interestHC, nameTelia, nameUcla);

  // C forwards to S, no change to forwarding hint
  BOOST_CHECK_EQUAL(linkCS->getFace(nodeC).getCounters().nOutInterests, 1);
  const Interest& interestCS = topo.getPcap(linkCS->getFace(nodeC)).sentInterests.at(0);
  CHECK_FH_EQUAL(interestCS, nameTelia, nameUcla);

  // S forwards to Q, forwarding hint stripped when Interest reaches producer region
  BOOST_CHECK_EQUAL(linkSQ->getFace(nodeS).getCounters().nOutInterests, 1);
  const Interest& interestSQ = topo.getPcap(linkSQ->getFace(nodeS)).sentInterests.at(0);
  BOOST_CHECK(interestSQ.getForwardingHint().empty());

  // Data is served by Q and reaches A
  BOOST_CHECK_EQUAL(producerQ->getForwarderFace().getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(consumerA->getForwarderFace().getCounters().nOutData, 1);
}

BOOST_AUTO_TEST_SUITE_END() // NdnsimTeliaUclaTopology

BOOST_AUTO_TEST_SUITE_END() // TestForwardingHint
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
