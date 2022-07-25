/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021 Regents of the University of California,
 *                         Arizona Board of Regents,
 *                         Colorado State University,
 *                         University Pierre & Marie Curie, Sorbonne University,
 *                         Washington University in St. Louis,
 *                         Beijing Institute of Technology,
 *                         The University of Memphis.
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

#include "ndn-cxx/util/notification-stream.hpp"
#include "ndn-cxx/util/dummy-client-face.hpp"

#include "tests/boost-test.hpp"
#include "tests/unit/io-key-chain-fixture.hpp"
#include "tests/unit/util/simple-notification.hpp"

namespace ndn {
namespace util {
namespace tests {

BOOST_AUTO_TEST_SUITE(Util)
BOOST_FIXTURE_TEST_SUITE(TestNotificationStream, ndn::tests::IoKeyChainFixture)

BOOST_AUTO_TEST_CASE(Post)
{
  DummyClientFace face(m_io, m_keyChain);
  NotificationStream<SimpleNotification> notificationStream(face,
                                                            "/localhost/nfd/NotificationStreamTest",
                                                            m_keyChain);

  SimpleNotification event1("msg1");
  notificationStream.postNotification(event1);

  advanceClocks(1_ms);

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData[0].getName(), "/localhost/nfd/NotificationStreamTest/seq=0");
  SimpleNotification decoded1;
  BOOST_CHECK_NO_THROW(decoded1.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(decoded1.getMessage(), "msg1");

  SimpleNotification event2("msg2");
  notificationStream.postNotification(event2);

  advanceClocks(1_ms);

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 2);
  BOOST_CHECK_EQUAL(face.sentData[1].getName(), "/localhost/nfd/NotificationStreamTest/seq=1");
  SimpleNotification decoded2;
  BOOST_CHECK_NO_THROW(decoded2.wireDecode(face.sentData[1].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(decoded2.getMessage(), "msg2");
}

BOOST_AUTO_TEST_SUITE_END() // TestNotificationStream
BOOST_AUTO_TEST_SUITE_END() // Util

} // namespace tests
} // namespace util
} // namespace ndn
