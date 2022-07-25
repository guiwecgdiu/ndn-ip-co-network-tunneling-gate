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

#include "ndn-cxx/name.hpp"

#include "tests/boost-test.hpp"

#include <unordered_map>

namespace ndn {
namespace tests {

using Component = name::Component;
using UriFormat = name::UriFormat;

BOOST_AUTO_TEST_SUITE(TestName)

// ---- encoding, decoding, and URI ----

BOOST_AUTO_TEST_CASE(EncodeDecode)
{
  std::string uri = "/Emid/25042=P3/.../..../%1C%9F/"
                    "sha256digest=0415e3624a151850ac686c84f155f29808c0dd73819aa4a4c20be73a4d8a874c";
  Name name(uri);
  BOOST_CHECK_EQUAL(name.size(), 6);
  BOOST_CHECK_EQUAL(name[0], Component("Emid"));
  BOOST_CHECK_EQUAL(name[1], Component("FD61D2025033"_block));
  BOOST_CHECK_EQUAL(name[2], Component(""));
  BOOST_CHECK_EQUAL(name[3], Component("."));
  BOOST_CHECK_EQUAL(name[4], Component("\x1C\x9F"));
  BOOST_CHECK(name[5].isImplicitSha256Digest());

  BOOST_CHECK_EQUAL(name.toUri(UriFormat::CANONICAL),
                    "/8=Emid/25042=P3/8=.../8=..../8=%1C%9F/"
                    "1=%04%15%E3bJ%15%18P%AChl%84%F1U%F2%98%08%C0%DDs%81%9A%A4%A4%C2%0B%E7%3AM%8A%87L");

  Block wire = name.wireEncode();
  BOOST_CHECK_EQUAL(wire,
    "0737 0804456D6964 FD61D2025033 0800 08012E 08021C9F "
    "01200415E3624A151850AC686C84F155F29808C0DD73819AA4A4C20BE73A4D8A874C"_block);

  Name decoded(wire);
  BOOST_CHECK_EQUAL(decoded, name);
}

BOOST_AUTO_TEST_CASE(ParseUri)
{
  // canonical URI
  BOOST_CHECK_EQUAL(Name("/8=hello/8=world").toUri(), "/hello/world");

  // URI with correct scheme
  BOOST_CHECK_EQUAL(Name("ndn:/hello/world").toUri(), "/hello/world");

  // URI with incorrect scheme: auto-corrected
  BOOST_CHECK_EQUAL(Name("ncc:/hello/world").toUri(), "/hello/world");

  // URI with authority: authority ignored
  BOOST_CHECK_EQUAL(Name("//authority/hello/world").toUri(), "/hello/world");
  BOOST_CHECK_EQUAL(Name("ndn://authority/hello/world").toUri(), "/hello/world");

  // URI containing unescaped characters: auto-corrected
  BOOST_CHECK_EQUAL(Name("/ hello\t/\tworld \r\n").toUri(), "/%20hello%09/%09world%20%0D%0A");
  BOOST_CHECK_EQUAL(Name("/hello/world/  ").toUri(), "/hello/world/%20%20");
  BOOST_CHECK_EQUAL(Name("/:?#[]@").toUri(), "/%3A%3F%23%5B%5D%40");

  // URI not starting with '/': accepted as PartialName
  BOOST_CHECK_EQUAL(Name("").toUri(), "/");
  BOOST_CHECK_EQUAL(Name(" ").toUri(), "/%20");
  BOOST_CHECK_EQUAL(Name("  /hello/world").toUri(), "/%20%20/hello/world");
  BOOST_CHECK_EQUAL(Name("hello/world").toUri(), "/hello/world");

  // URI ending with '/': auto-corrected
  BOOST_CHECK_EQUAL(Name("/hello/world/").toUri(), "/hello/world");

  // URI containing bad component: rejected
  BOOST_CHECK_THROW(Name("/hello//world"), name::Component::Error);
  BOOST_CHECK_THROW(Name("/hello/./world"), name::Component::Error);
  BOOST_CHECK_THROW(Name("/hello/../world"), name::Component::Error);
}

BOOST_AUTO_TEST_CASE(DeepCopy)
{
  Name n1("/hello/world");
  Name n2 = n1.deepCopy();

  BOOST_CHECK_EQUAL(n1, n2);
  BOOST_CHECK_NE(&n1.wireEncode(), &n2.wireEncode());

  EncodingBuffer buffer(1024, 0);
  n1.wireEncode(buffer);
  Name n3(buffer.block());

  BOOST_CHECK_EQUAL(n1, n3);
  BOOST_CHECK_EQUAL(n3.wireEncode().getBuffer()->size(), 1024);
  n3 = n3.deepCopy();

  BOOST_CHECK_LT(n3.wireEncode().size(), 1024);
  BOOST_CHECK_EQUAL(n3.wireEncode().getBuffer()->size(), n3.wireEncode().size());
}

// ---- access ----

BOOST_AUTO_TEST_CASE(At)
{
  Name name("/hello/5=NDN");

  BOOST_CHECK_EQUAL(name.at(0), name::Component("080568656C6C6F"_block));
  BOOST_CHECK_EQUAL(name.at(1), name::Component("05034E444E"_block));
  BOOST_CHECK_EQUAL(name.at(-1), name::Component("05034E444E"_block));
  BOOST_CHECK_EQUAL(name.at(-2), name::Component("080568656C6C6F"_block));

  BOOST_CHECK_THROW(name.at(2), Name::Error);
  BOOST_CHECK_THROW(name.at(-3), Name::Error);
}

BOOST_AUTO_TEST_CASE(SubName)
{
  Name name("/hello/world");

  BOOST_CHECK_EQUAL("/hello/world", name.getSubName(0));
  BOOST_CHECK_EQUAL("/world", name.getSubName(1));
  BOOST_CHECK_EQUAL("/hello", name.getSubName(0, 1));
}

BOOST_AUTO_TEST_CASE(SubNameNegativeIndex)
{
  Name name("/first/second/third/last");

  BOOST_CHECK_EQUAL("/last", name.getSubName(-1));
  BOOST_CHECK_EQUAL("/third/last", name.getSubName(-2));
  BOOST_CHECK_EQUAL("/second", name.getSubName(-3, 1));
}

BOOST_AUTO_TEST_CASE(SubNameOutOfRangeIndexes)
{
  Name name("/first/second/last");
  // No length
  BOOST_CHECK_EQUAL("/first/second/last", name.getSubName(-10));
  BOOST_CHECK_EQUAL("/", name.getSubName(10));

  // Starting after the max position
  BOOST_CHECK_EQUAL("/", name.getSubName(10, 1));
  BOOST_CHECK_EQUAL("/", name.getSubName(10, 10));

  // Not enough components
  BOOST_CHECK_EQUAL("/second/last", name.getSubName(1, 10));
  BOOST_CHECK_EQUAL("/last", name.getSubName(-1, 10));

  // Start before first
  BOOST_CHECK_EQUAL("/first/second", name.getSubName(-10, 2));
  BOOST_CHECK_EQUAL("/first/second/last", name.getSubName(-10, 10));
}

// ---- iterators ----

BOOST_AUTO_TEST_CASE(ForwardIterator)
{
  name::Component comps[] {
    name::Component("A"),
    name::Component("B"),
    name::Component("C"),
    name::Component("D")
  };

  Name n0;
  BOOST_CHECK_EQUAL_COLLECTIONS(n0.begin(), n0.end(), comps, comps + 0);

  Name n4("/A/B/C/D");
  BOOST_CHECK_EQUAL_COLLECTIONS(n4.begin(), n4.end(), comps, comps + 4);
}

BOOST_AUTO_TEST_CASE(ReverseIterator)
{
  name::Component comps[] {
    name::Component("D"),
    name::Component("C"),
    name::Component("B"),
    name::Component("A")
  };

  Name n0;
  BOOST_CHECK_EQUAL_COLLECTIONS(n0.rbegin(), n0.rend(), comps, comps + 0);

  Name n4("/A/B/C/D");
  BOOST_CHECK_EQUAL_COLLECTIONS(n4.rbegin(), n4.rend(), comps, comps + 4);
}

// ---- modifiers ----

BOOST_AUTO_TEST_CASE(SetComponent)
{
  Name name("/A/B");
  BOOST_CHECK_EQUAL(name.wireEncode(), "0706 080141 080142"_block);
  BOOST_CHECK_EQUAL(name.hasWire(), true);

  // pass by const lvalue ref
  const Component c("C");
  name.set(0, c);
  BOOST_CHECK_EQUAL(name.hasWire(), false);
  BOOST_CHECK_EQUAL(name.wireEncode(), "0706 080143 080142"_block);

  // pass by rvalue ref
  Component d("D");
  name.set(1, std::move(d));
  BOOST_CHECK_EQUAL(name.hasWire(), false);
  BOOST_CHECK_EQUAL(name.wireEncode(), "0706 080143 080144"_block);

  // negative index
  name.set(-1, Component("E"));
  BOOST_CHECK_EQUAL(name.wireEncode(), "0706 080143 080145"_block);
}

BOOST_AUTO_TEST_CASE(AppendComponent)
{
  Name name;
  BOOST_CHECK_EQUAL(name.wireEncode(), "0700"_block);

  name.append(Component("Emid"));
  BOOST_CHECK_EQUAL(name.wireEncode(), "0706 0804456D6964"_block);

  name.append(25042, {'P', '3'});
  BOOST_CHECK_EQUAL(name.wireEncode(), "070C 0804456D6964 FD61D2025033"_block);

  const uint8_t arr[] = {'.'};
  name.append(arr);
  BOOST_CHECK_EQUAL(name.wireEncode(), "070F 0804456D6964 FD61D2025033 08012E"_block);

  const std::vector<uint8_t> vec{0x28, 0xF0, 0xA3, 0x6B};
  name.append(16, vec.begin(), vec.end());
  BOOST_CHECK_EQUAL(name.wireEncode(), "0715 0804456D6964 FD61D2025033 08012E 100428F0A36B"_block);

  name.clear();
  BOOST_CHECK_EQUAL(name.wireEncode(), "0700"_block);

  name.append(vec.begin(), vec.end());
  BOOST_CHECK_EQUAL(name.wireEncode(), "0706 080428F0A36B"_block);

  name.append("xKh");
  BOOST_CHECK_EQUAL(name.wireEncode(), "070B 080428F0A36B 0803784B68"_block);
}

BOOST_AUTO_TEST_CASE(AppendPartialName)
{
  Name name("/A/B");
  name.append(PartialName("/6=C/D"))
      .append(PartialName("/E"));
  BOOST_CHECK_EQUAL(name.wireEncode(), "070F 080141 080142 060143 080144 080145"_block);

  name = "/A/B";
  name.append(name);
  BOOST_CHECK_EQUAL(name.wireEncode(), "070C 080141 080142 080141 080142"_block);
}

BOOST_AUTO_TEST_CASE(AppendNumber)
{
  Name name;
  for (uint32_t i = 0; i < 10; i++) {
    name.appendNumber(i);
  }
  BOOST_CHECK_EQUAL(name.size(), 10);

  for (uint32_t i = 0; i < 10; i++) {
    BOOST_CHECK_EQUAL(name[i].toNumber(), i);
  }
}

BOOST_AUTO_TEST_CASE(AppendParametersSha256Digest)
{
  auto digest = make_shared<Buffer>(32);

  Name name("/P");
  name.appendParametersSha256Digest(digest); // ConstBufferPtr overload
  BOOST_CHECK_EQUAL(name.wireEncode(),
                    "0725 080150 02200000000000000000000000000000000000000000000000000000000000000000"_block);

  name = "/P";
  name.appendParametersSha256Digest(*digest); // span overload
  BOOST_CHECK_EQUAL(name.wireEncode(),
                    "0725 080150 02200000000000000000000000000000000000000000000000000000000000000000"_block);

  name = "/P";
  name.appendParametersSha256DigestPlaceholder();
  BOOST_CHECK_EQUAL(name.wireEncode(),
                    "0725 080150 0220E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855"_block);
}

BOOST_AUTO_TEST_CASE(AppendTypedComponent)
{
  // TestNameComponent/NamingConvention provides additional coverage for these methods,
  // including verification of the wire format.

  Name name;
  uint64_t number;

  BOOST_CHECK_NO_THROW(number = name.appendSegment(30923).at(-1).toSegment());
  BOOST_TEST(number == 30923);

  BOOST_CHECK_NO_THROW(number = name.appendByteOffset(41880).at(-1).toByteOffset());
  BOOST_TEST(number == 41880);

  auto before = time::toUnixTimestamp(time::system_clock::now());
  BOOST_CHECK_NO_THROW(number = name.appendVersion().at(-1).toVersion());
  auto after = time::toUnixTimestamp(time::system_clock::now());
  BOOST_TEST(number >= before.count());
  BOOST_TEST(number <= after.count());

  BOOST_CHECK_NO_THROW(number = name.appendVersion(25912).at(-1).toVersion());
  BOOST_TEST(number == 25912);

  const auto tp = time::system_clock::now();
  time::system_clock::TimePoint tp2;
  BOOST_CHECK_NO_THROW(tp2 = name.appendTimestamp(tp).at(-1).toTimestamp());
  BOOST_TEST(time::abs(tp2 - tp) <= 1_us);

  BOOST_CHECK_NO_THROW(number = name.appendSequenceNumber(11676).at(-1).toSequenceNumber());
  BOOST_TEST(number == 11676);

  name.appendKeyword({0xab, 0xcd, 0xef});
  BOOST_TEST(name.at(-1) == Component::fromEscapedString("32=%AB%CD%EF"));

  name.appendKeyword("test-keyword");
  BOOST_TEST(name.at(-1) == Component::fromEscapedString("32=test-keyword"));
}

BOOST_AUTO_TEST_CASE(EraseComponent)
{
  Name name("/A/B/C");
  BOOST_CHECK_EQUAL(name.wireEncode(), "0709 080141 080142 080143"_block);
  BOOST_CHECK_EQUAL(name.hasWire(), true);

  name.erase(-2);
  BOOST_CHECK_EQUAL(name.size(), 2);
  BOOST_CHECK_EQUAL(name.hasWire(), false);
  BOOST_CHECK_EQUAL(name.wireEncode(), "0706 080141 080143"_block);

  name.erase(1);
  BOOST_CHECK_EQUAL(name.size(), 1);
  BOOST_CHECK_EQUAL(name.hasWire(), false);
  BOOST_CHECK_EQUAL(name.wireEncode(), "0703 080141"_block);

  name.erase(0);
  BOOST_CHECK_EQUAL(name.size(), 0);
  BOOST_CHECK_EQUAL(name.hasWire(), false);
  BOOST_CHECK_EQUAL(name.wireEncode(), "0700"_block);
}

BOOST_AUTO_TEST_CASE(Clear)
{
  Name name("/A/B/C");
  BOOST_CHECK_EQUAL(name.empty(), false);
  name.clear();
  BOOST_CHECK_EQUAL(name.empty(), true);
  BOOST_CHECK(name.begin() == name.end());
}

// ---- algorithms ----

BOOST_AUTO_TEST_CASE(GetSuccessor)
{
  BOOST_CHECK_EQUAL(Name().getSuccessor(), "/sha256digest=0000000000000000000000000000000000000000000000000000000000000000");
  BOOST_CHECK_EQUAL(Name("/sha256digest=0000000000000000000000000000000000000000000000000000000000000000").getSuccessor(),
                    "/sha256digest=0000000000000000000000000000000000000000000000000000000000000001");
  BOOST_CHECK_EQUAL(Name("/sha256digest=ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").getSuccessor(),
                    "/params-sha256=0000000000000000000000000000000000000000000000000000000000000000");
  BOOST_CHECK_EQUAL(Name("/params-sha256=0000000000000000000000000000000000000000000000000000000000000000").getSuccessor(),
                    "/params-sha256=0000000000000000000000000000000000000000000000000000000000000001");
  BOOST_CHECK_EQUAL(Name("/params-sha256=ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").getSuccessor(),
                    "/3=...");
  BOOST_CHECK_EQUAL(Name("/P/A").getSuccessor(), "/P/B");
  BOOST_CHECK_EQUAL(Name("/P/AAA").getSuccessor(), "/P/AAB");
  BOOST_CHECK_EQUAL(Name("/Q/...").getSuccessor(), "/Q/%00");
  BOOST_CHECK_EQUAL(Name("/Q/%FF").getSuccessor(), "/Q/%00%00");
  BOOST_CHECK_EQUAL(Name("/Q/%FE%FF").getSuccessor(), "/Q/%FF%00");
  BOOST_CHECK_EQUAL(Name("/Q/%FF%FF").getSuccessor(), "/Q/%00%00%00");
  BOOST_CHECK_EQUAL(Name("/P/3=A").getSuccessor(), "/P/3=B");
  BOOST_CHECK_EQUAL(Name("/P/3=AAA").getSuccessor(), "/P/3=AAB");
  BOOST_CHECK_EQUAL(Name("/Q/3=...").getSuccessor(), "/Q/3=%00");
  BOOST_CHECK_EQUAL(Name("/Q/3=%FF").getSuccessor(), "/Q/3=%00%00");
  BOOST_CHECK_EQUAL(Name("/Q/3=%FE%FF").getSuccessor(), "/Q/3=%FF%00");
  BOOST_CHECK_EQUAL(Name("/Q/3=%FF%FF").getSuccessor(), "/Q/3=%00%00%00");
}

BOOST_AUTO_TEST_CASE(IsPrefixOf)
{
  BOOST_CHECK(Name("/").isPrefixOf("/"));
  BOOST_CHECK(Name("/").isPrefixOf("/sha256digest=0000000000000000000000000000000000000000000000000000000000000000"));
  BOOST_CHECK(Name("/").isPrefixOf("/params-sha256=ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
  BOOST_CHECK(Name("/").isPrefixOf("/3=D"));
  BOOST_CHECK(Name("/").isPrefixOf("/F"));
  BOOST_CHECK(Name("/").isPrefixOf("/21426=AA"));

  BOOST_CHECK(Name("/B").isPrefixOf("/B"));
  BOOST_CHECK(Name("/B").isPrefixOf("/B/sha256digest=0000000000000000000000000000000000000000000000000000000000000000"));
  BOOST_CHECK(Name("/").isPrefixOf("/B/params-sha256=ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
  BOOST_CHECK(Name("/B").isPrefixOf("/B/3=D"));
  BOOST_CHECK(Name("/B").isPrefixOf("/B/F"));
  BOOST_CHECK(Name("/B").isPrefixOf("/B/21426=AA"));

  BOOST_CHECK(!Name("/C").isPrefixOf("/"));
  BOOST_CHECK(!Name("/C").isPrefixOf("/sha256digest=0000000000000000000000000000000000000000000000000000000000000000"));
  BOOST_CHECK(Name("/").isPrefixOf("/params-sha256=ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
  BOOST_CHECK(!Name("/C").isPrefixOf("/3=D"));
  BOOST_CHECK(!Name("/C").isPrefixOf("/F"));
  BOOST_CHECK(!Name("/C").isPrefixOf("/21426=AA"));
}

BOOST_AUTO_TEST_CASE(CompareOp)
{
  std::vector<Name> names = {
    Name("/"),
    Name("/sha256digest=0000000000000000000000000000000000000000000000000000000000000000"),
    Name("/sha256digest=0000000000000000000000000000000000000000000000000000000000000001"),
    Name("/sha256digest=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"),
    Name("/params-sha256=0000000000000000000000000000000000000000000000000000000000000000"),
    Name("/params-sha256=0000000000000000000000000000000000000000000000000000000000000001"),
    Name("/params-sha256=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"),
    Name("/3=..."),
    Name("/3=D"),
    Name("/3=F"),
    Name("/3=AA"),
    Name("/..."),
    Name("/D"),
    Name("/D/sha256digest=0000000000000000000000000000000000000000000000000000000000000000"),
    Name("/D/sha256digest=0000000000000000000000000000000000000000000000000000000000000001"),
    Name("/D/sha256digest=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"),
    Name("/D/params-sha256=0000000000000000000000000000000000000000000000000000000000000000"),
    Name("/D/params-sha256=0000000000000000000000000000000000000000000000000000000000000001"),
    Name("/D/params-sha256=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"),
    Name("/D/3=..."),
    Name("/D/3=D"),
    Name("/D/3=F"),
    Name("/D/3=AA"),
    Name("/D/..."),
    Name("/D/D"),
    Name("/D/F"),
    Name("/D/AA"),
    Name("/D/21426=..."),
    Name("/D/21426=D"),
    Name("/D/21426=F"),
    Name("/D/21426=AA"),
    Name("/F"),
    Name("/AA"),
    Name("/21426=..."),
    Name("/21426=D"),
    Name("/21426=F"),
    Name("/21426=AA"),
  };

  for (size_t i = 0; i < names.size(); ++i) {
    for (size_t j = 0; j < names.size(); ++j) {
      Name lhs = names[i];
      Name rhs = names[j];
      BOOST_CHECK_EQUAL(lhs == rhs, i == j);
      BOOST_CHECK_EQUAL(lhs != rhs, i != j);
      BOOST_CHECK_EQUAL(lhs <  rhs, i <  j);
      BOOST_CHECK_EQUAL(lhs <= rhs, i <= j);
      BOOST_CHECK_EQUAL(lhs >  rhs, i >  j);
      BOOST_CHECK_EQUAL(lhs >= rhs, i >= j);
    }
  }
}

BOOST_AUTO_TEST_CASE(CompareFunc)
{
  BOOST_CHECK_EQUAL(Name("/A")  .compare(Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/A")  .compare(Name("/B")),   0);
  BOOST_CHECK_GT   (Name("/B")  .compare(Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/A")  .compare(Name("/AA")),  0);
  BOOST_CHECK_GT   (Name("/AA") .compare(Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/A")  .compare(Name("/A/C")), 0);
  BOOST_CHECK_GT   (Name("/A/C").compare(Name("/A")),   0);

  BOOST_CHECK_EQUAL(Name("/Z/A/Y")  .compare(1, 1, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/B")),   0);
  BOOST_CHECK_GT   (Name("/Z/B/Y")  .compare(1, 1, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/AA")),  0);
  BOOST_CHECK_GT   (Name("/Z/AA/Y") .compare(1, 1, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/A/C")), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C/Y").compare(1, 2, Name("/A")),   0);

  BOOST_CHECK_EQUAL(Name("/Z/A")  .compare(1, Name::npos, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A")  .compare(1, Name::npos, Name("/B")),   0);
  BOOST_CHECK_GT   (Name("/Z/B")  .compare(1, Name::npos, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A")  .compare(1, Name::npos, Name("/AA")),  0);
  BOOST_CHECK_GT   (Name("/Z/AA") .compare(1, Name::npos, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A")  .compare(1, Name::npos, Name("/A/C")), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C").compare(1, Name::npos, Name("/A")),   0);

  BOOST_CHECK_EQUAL(Name("/Z/A/Y")  .compare(1, 1, Name("/X/A/W"),   1, 1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/B/W"),   1, 1), 0);
  BOOST_CHECK_GT   (Name("/Z/B/Y")  .compare(1, 1, Name("/X/A/W"),   1, 1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/AA/W"),  1, 1), 0);
  BOOST_CHECK_GT   (Name("/Z/AA/Y") .compare(1, 1, Name("/X/A/W"),   1, 1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/A/C/W"), 1, 2), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C/Y").compare(1, 2, Name("/X/A/W"),   1, 1), 0);

  BOOST_CHECK_EQUAL(Name("/Z/A/Y")  .compare(1, 1, Name("/X/A"),   1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/B"),   1), 0);
  BOOST_CHECK_GT   (Name("/Z/B/Y")  .compare(1, 1, Name("/X/A"),   1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/AA"),  1), 0);
  BOOST_CHECK_GT   (Name("/Z/AA/Y") .compare(1, 1, Name("/X/A"),   1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/A/C"), 1), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C/Y").compare(1, 2, Name("/X/A"),   1), 0);
}

BOOST_AUTO_TEST_CASE(UnorderedMap)
{
  std::unordered_map<Name, int> map;
  Name name1("/1");
  Name name2("/2");
  Name name3("/3");
  map[name1] = 1;
  map[name2] = 2;
  map[name3] = 3;

  BOOST_CHECK_EQUAL(map[name1], 1);
  BOOST_CHECK_EQUAL(map[name2], 2);
  BOOST_CHECK_EQUAL(map[name3], 3);
}

BOOST_AUTO_TEST_SUITE_END() // TestName

} // namespace tests
} // namespace ndn
