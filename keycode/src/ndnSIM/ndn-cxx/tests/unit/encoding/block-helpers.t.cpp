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
 */

#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ndn-cxx/name.hpp"

#include "tests/boost-test.hpp"

namespace ndn {
namespace encoding {
namespace tests {

BOOST_AUTO_TEST_SUITE(Encoding)
BOOST_AUTO_TEST_SUITE(TestBlockHelpers)

enum E8 : uint8_t
{
  E8_NONE
};

enum class EC8 : uint8_t
{
  NONE
};

enum E16 : uint16_t
{
  E16_NONE
};

enum class EC16 : uint16_t
{
  NONE
};

BOOST_AUTO_TEST_CASE(NonNegativeInteger)
{
  Block b = makeNonNegativeIntegerBlock(100, 1000);
  BOOST_CHECK_EQUAL(b.type(), 100);
  BOOST_CHECK_GT(b.value_size(), 0);
  BOOST_CHECK_EQUAL(readNonNegativeInteger(b), 1000);

  BOOST_CHECK_THROW(readNonNegativeInteger(Block()), tlv::Error);

  BOOST_CHECK_THROW(readNonNegativeIntegerAs<uint8_t>(b), tlv::Error);
  BOOST_CHECK_EQUAL(readNonNegativeIntegerAs<uint16_t>(b), 1000);
  BOOST_CHECK_EQUAL(readNonNegativeIntegerAs<size_t>(b), 1000);
  BOOST_CHECK_THROW(readNonNegativeIntegerAs<E8>(b), tlv::Error);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(readNonNegativeIntegerAs<E16>(b)), 1000);
  BOOST_CHECK_THROW(readNonNegativeIntegerAs<EC8>(b), tlv::Error);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(readNonNegativeIntegerAs<EC16>(b)), 1000);
}

BOOST_AUTO_TEST_CASE(Empty)
{
  Block b = makeEmptyBlock(200);
  BOOST_CHECK_EQUAL(b.type(), 200);
  BOOST_CHECK_EQUAL(b.value_size(), 0);
}

BOOST_AUTO_TEST_CASE(String)
{
  Block b = makeStringBlock(100, "Hello, world!");
  BOOST_CHECK_EQUAL(b.type(), 100);
  BOOST_CHECK_GT(b.value_size(), 0);
  BOOST_CHECK_EQUAL(readString(b), "Hello, world!");
}

BOOST_AUTO_TEST_CASE(Double)
{
  const double f = 0.25;
  Block b = makeDoubleBlock(100, f);
  BOOST_CHECK_EQUAL(b, "64083FD0000000000000"_block);

  EncodingEstimator estimator;
  size_t totalLength = prependDoubleBlock(estimator, 100, f);
  EncodingBuffer encoder(totalLength, 0);
  prependDoubleBlock(encoder, 100, f);
  BOOST_CHECK_EQUAL(encoder.block(), b);

  BOOST_CHECK_EQUAL(readDouble(b), f);
  BOOST_CHECK_THROW(readDouble("4200"_block), tlv::Error);
  BOOST_CHECK_THROW(readDouble("64043E800000"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(Binary)
{
  std::string buf1{1, 1, 1, 1};
  const uint8_t buf2[]{1, 1, 1, 1};
  std::list<uint8_t> buf3{1, 1, 1, 1};

  Block b1 = makeBinaryBlock(100, buf1.data(), buf1.size()); // char* overload
  Block b2 = makeBinaryBlock(100, buf2);                     // span overload
  Block b3 = makeBinaryBlock(100, buf1.begin(), buf1.end()); // fast encoding (random access iterator)
  Block b4 = makeBinaryBlock(100, buf3.begin(), buf3.end()); // slow encoding (general iterator)

  BOOST_CHECK_EQUAL(b1, b2);
  BOOST_CHECK_EQUAL(b1, b3);
  BOOST_CHECK_EQUAL(b1, b4);
  BOOST_CHECK_EQUAL(b1.type(), 100);
  BOOST_CHECK_EQUAL(b1.value_size(), buf1.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(b1.value_begin(), b1.value_end(), buf2, buf2 + sizeof(buf2));

  EncodingEstimator estimator;
  size_t length = prependBinaryBlock(estimator, 100, buf2);
  BOOST_CHECK_EQUAL(length, 6);

  EncodingBuffer encoder(length, 0);
  BOOST_CHECK_EQUAL(prependBinaryBlock(encoder, 100, buf2), 6);
  BOOST_CHECK_EQUAL(encoder.block(), b1);
}

BOOST_AUTO_TEST_CASE(PrependBlock)
{
  EncodingEstimator estimator;
  Block b1({0x01, 0x03, 0x00, 0x00, 0x00});
  size_t length = prependBlock(estimator, b1);
  BOOST_CHECK_EQUAL(length, 5);
  Block b2(100, b1);
  length += prependBlock(estimator, b2);
  BOOST_CHECK_EQUAL(length, 12);

  EncodingBuffer encoder(length, 0);
  BOOST_CHECK_EQUAL(prependBlock(encoder, b1), 5);
  BOOST_CHECK_EQUAL(prependBlock(encoder, b2), 7);
  BOOST_CHECK_EQUAL(encoder.size(), 12);
  encoder.prependVarNumber(encoder.size());
  encoder.prependVarNumber(200);
  BOOST_CHECK_EQUAL(encoder.block(), "C80C 64050103000000 0103000000"_block);
}

BOOST_AUTO_TEST_CASE(Nested)
{
  Name name("ndn:/Hello/World!");
  Block b1 = makeNestedBlock(100, name);

  BOOST_CHECK_EQUAL(b1.type(), 100);
  b1.parse();
  BOOST_CHECK_EQUAL(b1.elements().size(), 1);
  BOOST_CHECK_EQUAL(b1.elements().begin()->type(), name.wireEncode().type());
  BOOST_CHECK_EQUAL(*b1.elements().begin(), name.wireEncode());
}

BOOST_AUTO_TEST_CASE(NestedSequence)
{
  std::vector<Name> names;
  names.emplace_back("/A");
  names.emplace_back("/B");
  Block b1 = makeNestedBlock(100, names.begin(), names.end());

  BOOST_CHECK_EQUAL(b1.type(), 100);
  b1.parse();
  auto elements = b1.elements();
  BOOST_REQUIRE_EQUAL(elements.size(), 2);
  BOOST_CHECK_EQUAL(Name(elements[0]), names[0]);
  BOOST_CHECK_EQUAL(Name(elements[1]), names[1]);
}

BOOST_AUTO_TEST_SUITE_END() // TestBlockHelpers
BOOST_AUTO_TEST_SUITE_END() // Encoding

} // namespace tests
} // namespace encoding
} // namespace ndn
