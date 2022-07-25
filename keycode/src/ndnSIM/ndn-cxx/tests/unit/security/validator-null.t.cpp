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

#include "ndn-cxx/security/validator-null.hpp"

#include "tests/boost-test.hpp"
#include "tests/key-chain-fixture.hpp"

namespace ndn {
namespace security {
namespace tests {

using std::bind;

BOOST_AUTO_TEST_SUITE(Security)
BOOST_FIXTURE_TEST_SUITE(TestValidatorNull, ndn::tests::KeyChainFixture)

BOOST_AUTO_TEST_CASE(ValidateData)
{
  auto identity = m_keyChain.createIdentity("/TestValidator/Null");
  Data data("/Some/Other/Data/Name");
  m_keyChain.sign(data, signingByIdentity(identity));

  ValidatorNull validator;
  validator.validate(data,
                     bind([] { BOOST_CHECK_MESSAGE(true, "Validation should succeed"); }),
                     bind([] { BOOST_CHECK_MESSAGE(false, "Validation should not have failed"); }));
}

BOOST_AUTO_TEST_CASE(ValidateInterest)
{
  auto identity = m_keyChain.createIdentity("/TestValidator/Null");
  Interest interest("/Some/Other/Interest/Name");
  m_keyChain.sign(interest, signingByIdentity(identity));

  ValidatorNull validator;
  validator.validate(interest,
                     bind([] { BOOST_CHECK_MESSAGE(true, "Validation should succeed"); }),
                     bind([] { BOOST_CHECK_MESSAGE(false, "Validation should not have failed"); }));
}

BOOST_AUTO_TEST_SUITE_END() // TestValidatorNull
BOOST_AUTO_TEST_SUITE_END() // Security

} // namespace tests
} // namespace security
} // namespace ndn
