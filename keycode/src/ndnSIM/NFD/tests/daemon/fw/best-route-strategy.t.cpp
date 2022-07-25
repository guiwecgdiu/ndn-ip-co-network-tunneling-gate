/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
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
#include "common/global.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "strategy-tester.hpp"

namespace nfd {
namespace fw {
namespace tests {

using BestRouteStrategyTester = StrategyTester<BestRouteStrategy>;
NFD_REGISTER_STRATEGY(BestRouteStrategyTester);

BOOST_AUTO_TEST_SUITE(Fw)

class BestRouteStrategyFixture : public GlobalIoTimeFixture
{
protected:
  BestRouteStrategyFixture()
    : face1(make_shared<DummyFace>())
    , face2(make_shared<DummyFace>())
    , face3(make_shared<DummyFace>())
    , face4(make_shared<DummyFace>())
    , face5(make_shared<DummyFace>())
  {
    faceTable.add(face1);
    faceTable.add(face2);
    faceTable.add(face3);
    faceTable.add(face4);
    faceTable.add(face5);
  }

protected:
  FaceTable faceTable;
  Forwarder forwarder{faceTable};
  BestRouteStrategyTester strategy{forwarder};
  Fib& fib{forwarder.getFib()};
  Pit& pit{forwarder.getPit()};

  shared_ptr<DummyFace> face1;
  shared_ptr<DummyFace> face2;
  shared_ptr<DummyFace> face3;
  shared_ptr<DummyFace> face4;
  shared_ptr<DummyFace> face5;
};

BOOST_FIXTURE_TEST_SUITE(TestBestRouteStrategy, BestRouteStrategyFixture)

BOOST_AUTO_TEST_CASE(Forward)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face1, 10);
  fib.addOrUpdateNextHop(fibEntry, *face2, 20);
  fib.addOrUpdateNextHop(fibEntry, *face3, 30);

  shared_ptr<Interest> interest = makeInterest("ndn:/BzgFBchqA");
  shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

  const auto TICK = time::duration_cast<time::nanoseconds>(
                      BestRouteStrategy::RETX_SUPPRESSION_INITIAL * 0.1);

  // first Interest goes to nexthop with lowest FIB cost,
  // however face1 is downstream so it cannot be used
  pitEntry->insertOrUpdateInRecord(*face1, *interest);
  strategy.afterReceiveInterest(*interest, FaceEndpoint(*face1, 0), pitEntry);
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 1);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory.back().outFaceId, face2->getId());

  // downstream retransmits frequently, but the strategy should not send Interests
  // more often than DEFAULT_MIN_RETX_INTERVAL
  scheduler::EventId retxFrom4Evt;
  size_t nSentLast = strategy.sendInterestHistory.size();
  auto timeSentLast = time::steady_clock::now();
  std::function<void()> periodicalRetxFrom4; // let periodicalRetxFrom4 lambda capture itself
  periodicalRetxFrom4 = [&] {
    pitEntry->insertOrUpdateInRecord(*face4, *interest);
    strategy.afterReceiveInterest(*interest, FaceEndpoint(*face4, 0), pitEntry);

    size_t nSent = strategy.sendInterestHistory.size();
    if (nSent > nSentLast) {
      BOOST_CHECK_EQUAL(nSent - nSentLast, 1);
      auto timeSent = time::steady_clock::now();
      BOOST_CHECK_GE(timeSent - timeSentLast, TICK * 8);
      nSentLast = nSent;
      timeSentLast = timeSent;
    }

    retxFrom4Evt = getScheduler().schedule(TICK * 5, periodicalRetxFrom4);
  };
  periodicalRetxFrom4();
  this->advanceClocks(TICK, BestRouteStrategy::RETX_SUPPRESSION_MAX * 16);
  retxFrom4Evt.cancel();

  // nexthops for accepted retransmissions: follow FIB cost,
  // later forward to an eligible upstream with earliest out-record
  BOOST_REQUIRE_GE(strategy.sendInterestHistory.size(), 6);
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[1].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[2].outFaceId, face3->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[3].outFaceId, face2->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[4].outFaceId, face1->getId());
  BOOST_CHECK_EQUAL(strategy.sendInterestHistory[5].outFaceId, face3->getId());

  fib.removeNextHop(fibEntry, *face1);

  strategy.sendInterestHistory.clear();
  for (int i = 0; i < 3; ++i) {
    this->advanceClocks(TICK, BestRouteStrategy::RETX_SUPPRESSION_MAX * 2);
    pitEntry->insertOrUpdateInRecord(*face5, *interest);
    strategy.afterReceiveInterest(*interest, FaceEndpoint(*face5, 0), pitEntry);
  }
  BOOST_REQUIRE_EQUAL(strategy.sendInterestHistory.size(), 3);
  BOOST_CHECK_NE(strategy.sendInterestHistory[0].outFaceId, face1->getId());
  BOOST_CHECK_NE(strategy.sendInterestHistory[1].outFaceId, face1->getId());
  BOOST_CHECK_NE(strategy.sendInterestHistory[2].outFaceId, face1->getId());
  // face1 cannot be used because it's gone from FIB entry
}

BOOST_AUTO_TEST_SUITE_END() // TestBestRouteStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
