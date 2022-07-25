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

#ifndef NFD_TESTS_DAEMON_FW_DUMMY_STRATEGY_HPP
#define NFD_TESTS_DAEMON_FW_DUMMY_STRATEGY_HPP

#include "fw/strategy.hpp"

namespace nfd {
namespace tests {

/** \brief Forwarding strategy for unit testing
 *
 *  Triggers are recorded but do nothing.
 *
 *  DummyStrategy registers itself as /dummy-strategy/<max-version>, so that it can be instantiated
 *  with any version number. Aliases can be created with the registerAs() function.
 */
class DummyStrategy : public fw::Strategy
{
public:
  static void
  registerAs(const Name& strategyName);

  static Name
  getStrategyName(uint64_t version = std::numeric_limits<uint64_t>::max());

  /**
   * \brief Constructor
   *
   * \p name is recorded unchanged as getInstanceName(), and will not automatically
   * gain a version number when instantiated without a version number.
   */
  explicit
  DummyStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  /**
   * \brief After receive Interest trigger
   *
   * If interestOutFace is not null, \p interest is forwarded to that face;
   * otherwise, rejectPendingInterest() is invoked.
   */
  void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterContentStoreHit(const Data& data, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  beforeSatisfyInterest(const Data& data, const FaceEndpoint& ingress,
                        const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveData(const Data& data, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterNewNextHop(const fib::NextHop& nextHop, const shared_ptr<pit::Entry>& pitEntry) override;

protected:
  /** \brief register an alias
   *  \tparam S subclass of DummyStrategy
   */
  template<typename S>
  static void
  registerAsImpl(const Name& strategyName)
  {
    if (!fw::Strategy::canCreate(strategyName)) {
      fw::Strategy::registerType<S>(strategyName);
    }
  }

public:
  int afterReceiveInterest_count = 0;
  int afterContentStoreHit_count = 0;
  int beforeSatisfyInterest_count = 0;
  int afterReceiveData_count = 0;
  int afterReceiveNack_count = 0;

  // a collection of names of PIT entries that afterNewNextHop() was called on
  std::vector<Name> afterNewNextHopCalls;

  shared_ptr<Face> interestOutFace;
};

/** \brief DummyStrategy with specific version
 */
template<uint64_t VERSION>
class VersionedDummyStrategy : public DummyStrategy
{
public:
  static void
  registerAs(const Name& strategyName)
  {
    DummyStrategy::registerAsImpl<VersionedDummyStrategy<VERSION>>(strategyName);
  }

  static Name
  getStrategyName()
  {
    return DummyStrategy::getStrategyName(VERSION);
  }

  /** \brief constructor
   *
   *  The strategy instance name is taken from \p name ; if it does not contain a version component,
   *  \p VERSION will be appended.
   */
  explicit
  VersionedDummyStrategy(Forwarder& forwarder, const Name& name = getStrategyName())
    : DummyStrategy(forwarder, Strategy::makeInstanceName(name, getStrategyName()))
  {
  }
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FW_DUMMY_STRATEGY_HPP
