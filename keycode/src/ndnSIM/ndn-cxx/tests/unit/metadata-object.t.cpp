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
 * @author Chavoosh Ghasemi <chghasemi@cs.arizona.edu>
 */

#include "ndn-cxx/metadata-object.hpp"

#include "tests/boost-test.hpp"
#include "tests/key-chain-fixture.hpp"

namespace ndn {
namespace tests {

class MetadataObjectFixture : public KeyChainFixture
{
public:
  MetadataObjectFixture()
    : versionedContentName(Name(baseContentName)
                           .appendVersion(342092199154ULL))
    , metadataFullName(Name(baseContentName)
                       .append(MetadataObject::getKeywordComponent())
                       .appendVersion(metadataVerNo)
                       .appendSegment(0))
  {
  }

protected:
  // content prefix
  const Name baseContentName = "/ndn/unit/tests";
  const Name versionedContentName;

  // metadata prefix
  const uint64_t metadataVerNo = 89400192181ULL;
  const Name metadataFullName;
};

BOOST_FIXTURE_TEST_SUITE(TestMetadataObject, MetadataObjectFixture)

BOOST_AUTO_TEST_CASE(EncodeDecode)
{
  MetadataObject metadata1;
  metadata1.setVersionedName(versionedContentName);

  // pass metadata version number
  const Data data1 = metadata1.makeData(metadataFullName.getPrefix(-2), m_keyChain,
                                        security::SigningInfo(), metadataVerNo);

  BOOST_CHECK_EQUAL(metadata1.getVersionedName(), versionedContentName);
  BOOST_CHECK_EQUAL(data1.getName(), metadataFullName);
  BOOST_CHECK_EQUAL(data1.getFreshnessPeriod(), 10_ms);

  // do not pass metadata version number
  metadata1.setVersionedName(versionedContentName);
  const Data data2 = metadata1.makeData(metadataFullName.getPrefix(-2), m_keyChain);
  BOOST_CHECK_NE(data2.getName()[-2].toVersion(), metadataVerNo);

  // construct a metadata object based on a valid metadata packet
  MetadataObject metadata2(data1);

  BOOST_CHECK_EQUAL(metadata2.getVersionedName(), versionedContentName);
  BOOST_CHECK(baseContentName.isPrefixOf(metadata2.makeData(metadataFullName.getPrefix(-2),
                                                            m_keyChain).getName()));
}

BOOST_AUTO_TEST_CASE(InvalidFormat)
{
  Data data;

  // invalid metadata name
  data.setName("/ndn/unit/test");
  BOOST_CHECK_EXCEPTION(MetadataObject{data}, tlv::Error, [] (const auto& e) {
    return e.what() == "Name /ndn/unit/test is not a valid MetadataObject name"s;
  });
  data.setName(Name("/ndn/unit/test").append(MetadataObject::getKeywordComponent()));
  BOOST_CHECK_EXCEPTION(MetadataObject{data}, tlv::Error, [] (const auto& e) {
    return e.what() == "Name /ndn/unit/test/32=metadata is not a valid MetadataObject name"s;
  });

  // invalid content type
  data.setName(Name("/ndn/unit/test")
               .append(MetadataObject::getKeywordComponent())
               .appendVersion()
               .appendSegment(0));
  data.setContentType(tlv::ContentType_Key);
  BOOST_CHECK_EXCEPTION(MetadataObject{data}, tlv::Error, [] (const auto& e) {
    return e.what() == "MetadataObject has invalid ContentType 2"s;
  });

  // empty content
  data.setContentType(tlv::ContentType_Blob);
  BOOST_CHECK_EXCEPTION(MetadataObject{data}, tlv::Error, [] (const auto& e) {
    return e.what() == "MetadataObject is empty"s;
  });

  // non-empty content with no name element
  data.setContent("F000"_block);
  BOOST_CHECK_EXCEPTION(MetadataObject{data}, tlv::Error, [] (const auto& e) {
    return e.what() == "No sub-element of type 7 found in block of type 21"s;
  });
}

BOOST_AUTO_TEST_CASE(KeywordComponent)
{
  BOOST_CHECK_EQUAL(MetadataObject::getKeywordComponent().wireEncode(),
                    "20 08 6D65746164617461"_block);
  BOOST_CHECK_EQUAL(MetadataObject::getKeywordComponent().toUri(name::UriFormat::CANONICAL),
                    "32=metadata");
}

BOOST_AUTO_TEST_CASE(IsValidName)
{
  // valid name
  Name name = Name("/ndn/unit/test")
              .append(MetadataObject::getKeywordComponent())
              .appendVersion()
              .appendSegment(0);
  BOOST_CHECK(MetadataObject::isValidName(name));

  // invalid names
  // segment component is missing
  BOOST_CHECK_EQUAL(MetadataObject::isValidName(name.getPrefix(-1)), false);

  // version component is missing
  BOOST_CHECK_EQUAL(MetadataObject::isValidName(name.getPrefix(-2)), false);

  // keyword component is missing
  BOOST_CHECK_EQUAL(MetadataObject::isValidName(name.getPrefix(-3)), false);

  // too short name
  BOOST_CHECK_EQUAL(MetadataObject::isValidName(name.getPrefix(-4)), false);

  // out-of-order segment and version components
  name = name.getPrefix(-2).appendSegment(0).appendVersion();
  BOOST_CHECK_EQUAL(MetadataObject::isValidName(name), false);

  // invalid keyword name component
  name = name.getPrefix(-3)
         .append(tlv::KeywordNameComponent, {'f', 'o', 'o'})
         .appendVersion()
         .appendSegment(0);
  BOOST_CHECK_EQUAL(MetadataObject::isValidName(name), false);
}

BOOST_AUTO_TEST_CASE(MakeDiscoveryInterest)
{
  Interest interest = MetadataObject::makeDiscoveryInterest(baseContentName);
  BOOST_CHECK_EQUAL(interest.getName(), Name(baseContentName).append(MetadataObject::getKeywordComponent()));
  BOOST_CHECK(interest.getCanBePrefix());
  BOOST_CHECK(interest.getMustBeFresh());
}

BOOST_AUTO_TEST_SUITE_END() // TestMetadataObject

} // namespace tests
} // namespace ndn
