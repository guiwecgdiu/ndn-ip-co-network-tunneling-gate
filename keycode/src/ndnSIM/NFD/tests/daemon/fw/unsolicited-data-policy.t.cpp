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

#include "fw/unsolicited-data-policy.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include <boost/logic/tribool.hpp>
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

class UnsolicitedDataPolicyFixture : public GlobalIoTimeFixture
{
protected:
  /** \tparam Policy policy type, or void to keep default policy
   */
  template<typename Policy>
  void
  setPolicy()
  {
    forwarder.setUnsolicitedDataPolicy(make_unique<Policy>());
  }

  bool
  isInCs(const Data& data)
  {
    using namespace boost::logic;

    tribool isFound = indeterminate;
    cs.find(Interest(data.getFullName()),
            [&] (auto&&...) { isFound = true; },
            [&] (auto&&...) { isFound = false; });

    this->advanceClocks(1_ms);
    BOOST_REQUIRE(!indeterminate(isFound));
    return bool(isFound);
  }

protected:
  FaceTable faceTable;
  Forwarder forwarder{faceTable};
  Cs& cs{forwarder.getCs()};
};

template<>
void
UnsolicitedDataPolicyFixture::setPolicy<void>()
{
  // void keeps the default policy
}

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestUnsolicitedDataPolicy, UnsolicitedDataPolicyFixture)

BOOST_AUTO_TEST_CASE(GetPolicyNames)
{
  std::set<std::string> policyNames = UnsolicitedDataPolicy::getPolicyNames();
  BOOST_CHECK_EQUAL(policyNames.count("drop-all"), 1);
  BOOST_CHECK_EQUAL(policyNames.count("admit-local"), 1);
  BOOST_CHECK_EQUAL(policyNames.count("admit-network"), 1);
  BOOST_CHECK_EQUAL(policyNames.count("admit-all"), 1);
}

template<typename Policy, bool shouldAdmitLocal, bool shouldAdmitNonLocal>
struct FaceScopePolicyTest
{
  using PolicyType = Policy;
  using ShouldAdmitLocal = std::integral_constant<bool, shouldAdmitLocal>;
  using ShouldAdmitNonLocal = std::integral_constant<bool, shouldAdmitNonLocal>;
};

using FaceScopePolicyTests = boost::mpl::vector<
  FaceScopePolicyTest<void, false, false>, // default policy
  FaceScopePolicyTest<DropAllUnsolicitedDataPolicy, false, false>,
  FaceScopePolicyTest<AdmitLocalUnsolicitedDataPolicy, true, false>,
  FaceScopePolicyTest<AdmitNetworkUnsolicitedDataPolicy, false, true>,
  FaceScopePolicyTest<AdmitAllUnsolicitedDataPolicy, true, true>
>;

BOOST_AUTO_TEST_CASE_TEMPLATE(FaceScopePolicy, T, FaceScopePolicyTests)
{
  setPolicy<typename T::PolicyType>();

  auto face1 = make_shared<DummyFace>("dummy://", "dummy://",
                                      ndn::nfd::FACE_SCOPE_LOCAL);
  faceTable.add(face1);

  auto data1 = makeData("/unsolicited-from-local");
  forwarder.onIncomingData(*data1, FaceEndpoint(*face1, 0));
  BOOST_CHECK_EQUAL(isInCs(*data1), T::ShouldAdmitLocal::value);

  auto face2 = make_shared<DummyFace>("dummy://", "dummy://",
                                      ndn::nfd::FACE_SCOPE_NON_LOCAL);
  faceTable.add(face2);

  auto data2 = makeData("/unsolicited-from-non-local");
  forwarder.onIncomingData(*data2, FaceEndpoint(*face2, 0));
  BOOST_CHECK_EQUAL(isInCs(*data2), T::ShouldAdmitNonLocal::value);
}

BOOST_AUTO_TEST_SUITE_END() // TestUnsolicitedDataPolicy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
