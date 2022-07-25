/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2022 Regents of the University of California.
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
 *
 * @author Eric Newberry <enewberry@email.arizona.edu>
 */

#include "ndn-cxx/lp/cache-policy.hpp"

#include "tests/boost-test.hpp"

namespace ndn {
namespace lp {
namespace tests {

BOOST_AUTO_TEST_SUITE(Lp)
BOOST_AUTO_TEST_SUITE(TestCachePolicy)

BOOST_AUTO_TEST_CASE(Encode)
{
  CachePolicy policy;
  policy.setPolicy(CachePolicyType::NO_CACHE);

  Block wire;
  BOOST_CHECK_NO_THROW(wire = policy.wireEncode());

  // Sample encoded value obtained with:
  // for (auto it = wire.begin(); it != wire.end(); ++it) {
  //   printf("0x%02x, ", *it);
  // }

  // Contains CachePolicyType::NO_CACHE
  static const uint8_t expectedBlock[] = {
    0xfd, 0x03, 0x34, 0x05, 0xfd, 0x03, 0x35, 0x01, 0x01
  };

  BOOST_CHECK_EQUAL_COLLECTIONS(expectedBlock, expectedBlock + sizeof(expectedBlock),
                                wire.begin(), wire.end());

  BOOST_CHECK_NO_THROW(policy.wireDecode(wire));
}

BOOST_AUTO_TEST_CASE(DecodeUnknownPolicyError)
{
  const Block wire({0xfd, 0x03, 0x34, 0x08, 0xfd, 0x03, 0x35, 0x04, 0x1f, 0xff, 0xff, 0xff});

  CachePolicy policy;
  BOOST_CHECK_THROW(policy.wireDecode(wire), CachePolicy::Error);
}

BOOST_AUTO_TEST_CASE(DecodeMissingPolicyError)
{
  const Block wire({0xfd, 0x03, 0x34, 0x00});

  CachePolicy policy;
  BOOST_CHECK_THROW(policy.wireDecode(wire), CachePolicy::Error);
}

BOOST_AUTO_TEST_CASE(DecodeInvalidPolicyError)
{
  const Block wire({0xfd, 0x03, 0x34, 0x05, 0xfd, 0x03, 0x35, 0x01, 0x00});

  CachePolicy policy;
  BOOST_CHECK_THROW(policy.wireDecode(wire), CachePolicy::Error);
}

BOOST_AUTO_TEST_CASE(Policy)
{
  CachePolicy policy;
  BOOST_CHECK_EQUAL(policy.getPolicy(), CachePolicyType::NONE);

  policy.setPolicy(CachePolicyType::NO_CACHE);
  BOOST_CHECK_EQUAL(policy.getPolicy(), CachePolicyType::NO_CACHE);
}

BOOST_AUTO_TEST_SUITE_END() // TestCachePolicy
BOOST_AUTO_TEST_SUITE_END() // Lp

} // namespace tests
} // namespace lp
} // namespace ndn
