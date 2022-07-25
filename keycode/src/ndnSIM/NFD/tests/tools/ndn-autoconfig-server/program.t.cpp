/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
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

#include "ndn-autoconfig-server/program.hpp"

#include "tests/test-common.hpp"
#include "tests/key-chain-fixture.hpp"

#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>

namespace ndn {
namespace tools {
namespace autoconfig_server {
namespace tests {

class AutoconfigServerFixture : public ::nfd::tests::KeyChainFixture
{
protected:
  void
  initialize(const Options& options)
  {
    program = make_unique<Program>(options, face, m_keyChain);
    face.processEvents(1_s);
    face.sentInterests.clear();
  }

protected:
  util::DummyClientFace face{{true, true}};
  unique_ptr<Program> program;
};

BOOST_AUTO_TEST_SUITE(NdnAutoconfigServer)
BOOST_FIXTURE_TEST_SUITE(TestProgram, AutoconfigServerFixture)

BOOST_AUTO_TEST_CASE(HubData)
{
  Options options;
  options.hubFaceUri = FaceUri("udp4://192.0.2.1:6363");
  this->initialize(options);

  Interest interest("/localhop/ndn-autoconf/hub");
  interest.setCanBePrefix(true);
  interest.setMustBeFresh(true);
  face.receive(interest);
  face.processEvents(1_s);

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  const Name& dataName = face.sentData[0].getName();
  BOOST_CHECK_EQUAL(dataName.size(), interest.getName().size() + 1);
  BOOST_CHECK(interest.getName().isPrefixOf(dataName));
  BOOST_CHECK(dataName.at(-1).isVersion());

  // interest2 asks for a different version, and should not be responded
  Interest interest2(Name(interest.getName()).appendVersion(dataName.at(-1).toVersion() - 1));
  interest2.setCanBePrefix(false);
  face.receive(interest2);
  face.processEvents(1_s);
  BOOST_CHECK_EQUAL(face.sentData.size(), 1);
}

BOOST_AUTO_TEST_CASE(RoutablePrefixesDataset)
{
  Options options;
  options.hubFaceUri = FaceUri("udp4://192.0.2.1:6363");
  const size_t nRoutablePrefixes = 2000;
  for (size_t i = 0; i < nRoutablePrefixes; ++i) {
    options.routablePrefixes.push_back(Name("/PREFIX").appendNumber(i));
  }
  this->initialize(options);

  util::DummyClientFace clientFace(face.getIoService());
  clientFace.linkTo(face);

  Name baseName("/localhop/nfd/rib/routable-prefixes");
  auto fetcher = util::SegmentFetcher::start(clientFace, Interest(baseName),
                                             security::getAcceptAllValidator());
  fetcher->afterSegmentReceived.connect([baseName] (const Data& data) {
    const Name& dataName = data.getName();
    BOOST_CHECK_EQUAL(dataName.size(), baseName.size() + 2);
    BOOST_CHECK(dataName.at(-2).isVersion());
    BOOST_CHECK(dataName.at(-1).isSegment());
  });
  fetcher->onError.connect([] (auto, const auto& msg) { BOOST_FAIL(msg); });

  bool isComplete = false;
  fetcher->onComplete.connect([&] (auto buffer) {
    Block payload(tlv::Content, std::move(buffer));
    payload.parse();
    BOOST_REQUIRE_EQUAL(payload.elements_size(), nRoutablePrefixes);
    for (size_t i = 0; i < nRoutablePrefixes; ++i) {
      BOOST_CHECK_EQUAL(Name(payload.elements()[i]), options.routablePrefixes[i]);
    }
    isComplete = true;
  });

  face.processEvents(1_s);
  BOOST_CHECK(isComplete);
}

BOOST_AUTO_TEST_CASE(RoutablePrefixesDisabled)
{
  Options options;
  options.hubFaceUri = FaceUri("udp4://192.0.2.1:6363");
  this->initialize(options);

  Interest interest("/localhop/nfd/rib/routable-prefixes");
  interest.setCanBePrefix(true);
  face.receive(interest);
  face.processEvents(1_s);
  BOOST_CHECK_EQUAL(face.sentData.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestProgram
BOOST_AUTO_TEST_SUITE_END() // NdnAutoconfigServer

} // namespace tests
} // namespace autoconfig_server
} // namespace tools
} // namespace ndn
