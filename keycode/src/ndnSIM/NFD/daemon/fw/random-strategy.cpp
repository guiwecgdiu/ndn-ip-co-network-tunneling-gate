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

#include "random-strategy.hpp"
#include "algorithm.hpp"

#include <ndn-cxx/util/random.hpp>

namespace nfd {
namespace fw {

NFD_REGISTER_STRATEGY(RandomStrategy);
NFD_LOG_INIT(RandomStrategy);

RandomStrategy::RandomStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , ProcessNackTraits(this)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("RandomStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "RandomStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
RandomStrategy::getStrategyName()
{
  static const auto strategyName = Name("/localhost/nfd/strategy/random").appendVersion(1);
  return strategyName;
}

void
RandomStrategy::afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                                     const shared_ptr<pit::Entry>& pitEntry)
{
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  fib::NextHopList nhs;

  std::copy_if(fibEntry.getNextHops().begin(), fibEntry.getNextHops().end(), std::back_inserter(nhs),
               [&] (const auto& nh) { return isNextHopEligible(ingress.face, interest, nh, pitEntry); });

  if (nhs.empty()) {
    NFD_LOG_DEBUG(interest << " from=" << ingress << " no nexthop");

    lp::NackHeader nackHeader;
    nackHeader.setReason(lp::NackReason::NO_ROUTE);
    this->sendNack(nackHeader, ingress.face, pitEntry);
    this->rejectPendingInterest(pitEntry);
    return;
  }

  std::shuffle(nhs.begin(), nhs.end(), ndn::random::getRandomNumberEngine());
  this->sendInterest(interest, nhs.front().getFace(), pitEntry);
}

void
RandomStrategy::afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                                 const shared_ptr<pit::Entry>& pitEntry)
{
  this->processNack(nack, ingress.face, pitEntry);
}

} // namespace fw
} // namespace nfd
