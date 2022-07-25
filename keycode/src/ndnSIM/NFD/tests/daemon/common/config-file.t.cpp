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

#include "common/config-file.hpp"

#include "tests/test-common.hpp"

#include <boost/property_tree/info_parser.hpp>
#include <fstream>
#include <sstream>

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestConfigFile)

static const std::string CONFIG = R"CONFIG(
  a
  {
    akey avalue
  }
  b
  {
    bkey bvalue
  }
)CONFIG";

// counts of the respective section counts in config_example.info
const int CONFIG_N_A_SECTIONS = 1;
const int CONFIG_N_B_SECTIONS = 1;

class DummySubscriber
{
public:
  DummySubscriber(ConfigFile& config, int nASections, int nBSections, bool expectDryRun)
    : m_nASections(nASections)
    , m_nBSections(nBSections)
    , m_nRemainingACallbacks(nASections)
    , m_nRemainingBCallbacks(nBSections)
    , m_expectDryRun(expectDryRun)
  {
  }

  void
  onA(const ConfigSection& section, bool isDryRun)
  {
    BOOST_CHECK_EQUAL(isDryRun, m_expectDryRun);
    --m_nRemainingACallbacks;
  }

  void
  onB(const ConfigSection& section, bool isDryRun)
  {
    BOOST_CHECK_EQUAL(isDryRun, m_expectDryRun);
    --m_nRemainingBCallbacks;
  }

  bool
  allCallbacksFired() const
  {
    return m_nRemainingACallbacks == 0 &&
      m_nRemainingBCallbacks == 0;
  }

  bool
  noCallbacksFired() const
  {
    return m_nRemainingACallbacks == m_nASections &&
      m_nRemainingBCallbacks == m_nBSections;
  }

private:
  int m_nASections;
  int m_nBSections;
  int m_nRemainingACallbacks;
  int m_nRemainingBCallbacks;
  bool m_expectDryRun;
};

class DummyAllSubscriber : public DummySubscriber
{
public:
  explicit
  DummyAllSubscriber(ConfigFile& config, bool expectDryRun = false)
    : DummySubscriber(config, CONFIG_N_A_SECTIONS, CONFIG_N_B_SECTIONS, expectDryRun)
  {
    config.addSectionHandler("a", std::bind(&DummySubscriber::onA, this, _1, _2));
    config.addSectionHandler("b", std::bind(&DummySubscriber::onB, this, _1, _2));
  }
};

class DummyOneSubscriber : public DummySubscriber
{
public:
  DummyOneSubscriber(ConfigFile& config, const std::string& sectionName, bool expectDryRun = false)
    : DummySubscriber(config,
                      (sectionName == "a"),
                      (sectionName == "b"),
                      expectDryRun)
  {
    if (sectionName == "a") {
      config.addSectionHandler(sectionName, std::bind(&DummySubscriber::onA, this, _1, _2));
    }
    else if (sectionName == "b") {
      config.addSectionHandler(sectionName, std::bind(&DummySubscriber::onB, this, _1, _2));
    }
    else {
      BOOST_FAIL("Test setup error: Unexpected section name '" << sectionName << "'");
    }
  }
};

class DummyNoSubscriber : public DummySubscriber
{
public:
  DummyNoSubscriber(ConfigFile& config, bool expectDryRun)
    : DummySubscriber(config, 0, 0, expectDryRun)
  {
  }
};

BOOST_AUTO_TEST_CASE(ParseFromStream)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::ifstream input("tests/daemon/common/config_example.info");
  BOOST_REQUIRE(input.is_open());

  file.parse(input, false, "config_example.info");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromEmptyStream)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::istringstream input;

  file.parse(input, false, "empty");
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromStreamDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  std::ifstream input("tests/daemon/common/config_example.info");
  BOOST_REQUIRE(input.is_open());

  file.parse(input, true, "tests/daemon/common/config_example.info");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromConfigSection)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  std::istringstream input(CONFIG);
  ConfigSection section;
  boost::property_tree::read_info(input, section);

  file.parse(section, false, "dummy-config");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromString)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse(CONFIG, false, "dummy-config");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromEmptyString)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse("", false, "empty");
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromMalformedString)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  const std::string malformed = R"CONFIG(
    a
    {
      akey avalue
    }
    b
      bkey bvalue
    }
  )CONFIG";

  BOOST_CHECK_THROW(file.parse(malformed, false, "dummy-config"), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromStringDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  file.parse(CONFIG, true, "dummy-config");
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilename)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  file.parse("tests/daemon/common/config_example.info", false);
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilenameNonExistent)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse("i_made_this_up.info", false), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilenameMalformed)
{
  ConfigFile file;
  DummyAllSubscriber sub(file);

  BOOST_CHECK_THROW(file.parse("tests/daemon/common/config_malformed.info", false), ConfigFile::Error);
  BOOST_CHECK(sub.noCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseFromFilenameDryRun)
{
  ConfigFile file;
  DummyAllSubscriber sub(file, true);

  file.parse("tests/daemon/common/config_example.info", true);
  BOOST_CHECK(sub.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ReplaceSubscriber)
{
  ConfigFile file;
  DummyAllSubscriber sub1(file);
  DummyAllSubscriber sub2(file);

  file.parse(CONFIG, false, "dummy-config");

  BOOST_CHECK(sub1.noCallbacksFired());
  BOOST_CHECK(sub2.allCallbacksFired());
}

class MissingCallbackFixture
{
public:
  void
  checkMissingHandler(const std::string& filename,
                      const std::string& sectionName,
                      const ConfigSection& section,
                      bool isDryRun)
  {
    m_missingFired = true;
  }

protected:
  bool m_missingFired = false;
};

BOOST_FIXTURE_TEST_CASE(UncoveredSections, MissingCallbackFixture)
{
  ConfigFile file;
  BOOST_REQUIRE_THROW(file.parse(CONFIG, false, "dummy-config"), ConfigFile::Error);

  ConfigFile permissiveFile(std::bind(&MissingCallbackFixture::checkMissingHandler,
                                      this, _1, _2, _3, _4));
  DummyOneSubscriber subA(permissiveFile, "a");

  BOOST_REQUIRE_NO_THROW(permissiveFile.parse(CONFIG, false, "dummy-config"));
  BOOST_CHECK(subA.allCallbacksFired());
  BOOST_CHECK(m_missingFired);
}

BOOST_AUTO_TEST_CASE(CoveredByPartialSubscribers)
{
  ConfigFile file;
  DummyOneSubscriber subA(file, "a");
  DummyOneSubscriber subB(file, "b");

  file.parse(CONFIG, false, "dummy-config");

  BOOST_CHECK(subA.allCallbacksFired());
  BOOST_CHECK(subB.allCallbacksFired());
}

BOOST_AUTO_TEST_CASE(ParseNumber)
{
  std::istringstream input(R"CONFIG(
    a -125
    b 156
    c 100000
    d efg
    e 123.456e-10
    f 123abc
    g ""
    h " -1"
  )CONFIG");
  ConfigSection section;
  boost::property_tree::read_info(input, section);

  // Read various types and ensure they return the correct value
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<short>(section.get_child("a"), "a", "section"), -125);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<int>(section.get_child("a"), "a", "section"), -125);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<long>(section.get_child("a"), "a", "section"), -125);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<double>(section.get_child("a"), "a", "section"), -125.0);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<float>(section.get_child("a"), "a", "section"), -125.0);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<short>(section.get_child("b"), "b", "section"), 156);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<int>(section.get_child("b"), "b", "section"), 156);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<long>(section.get_child("b"), "b", "section"), 156);
  BOOST_CHECK_CLOSE(ConfigFile::parseNumber<double>(section.get_child("e"), "e", "section"), 123.456e-10, 0.0001);
  BOOST_CHECK_CLOSE(ConfigFile::parseNumber<float>(section.get_child("e"), "e", "section"), 123.456e-10, 0.0001);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<int>(section.get_child("h"), "h", "section"), -1);

  // Check throw on out of range
  BOOST_CHECK_THROW(ConfigFile::parseNumber<int16_t>(section.get_child("c"), "c", "section"),
                    ConfigFile::Error);

  // Check throw on non-integer for integer types
  BOOST_CHECK_THROW(ConfigFile::parseNumber<int>(section.get_child("d"), "d", "section"),
                    ConfigFile::Error);
  BOOST_CHECK_THROW(ConfigFile::parseNumber<int>(section.get_child("e"), "e", "section"),
                    ConfigFile::Error);
  BOOST_CHECK_THROW(ConfigFile::parseNumber<int>(section.get_child("f"), "f", "section"),
                    ConfigFile::Error);
  BOOST_CHECK_THROW(ConfigFile::parseNumber<int>(section.get_child("g"), "g", "section"),
                    ConfigFile::Error);

  // Should throw exception if try to read unsigned from negative value
  BOOST_CHECK_THROW(ConfigFile::parseNumber<uint16_t>(section.get_child("a"), "a", "section"),
                    ConfigFile::Error);
  BOOST_CHECK_EQUAL(ConfigFile::parseNumber<uint16_t>(section.get_child("b"), "b", "section"), 156);
  BOOST_CHECK_THROW(ConfigFile::parseNumber<uint32_t>(section.get_child("h"), "h", "section"),
                    ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(CheckRange)
{
  {
    uint32_t value = 8000;
    uint32_t min = 8000;
    uint32_t max = 8999;
    ConfigFile::checkRange(value, min, max, "key", "section");
  }

  {
    size_t value = 8999;
    size_t min = 8000;
    size_t max = 8999;
    ConfigFile::checkRange(value, min, max, "key", "section");
  }

  {
    int64_t value = -7000;
    int64_t min = -7999;
    int64_t max = 1000;
    ConfigFile::checkRange(value, min, max, "key", "section");
  }

  {
    uint32_t value = 7999;
    uint32_t min = 8000;
    uint32_t max = 8999;
    BOOST_CHECK_THROW(ConfigFile::checkRange(value, min, max, "key", "section"), ConfigFile::Error);
  }

  {
    int16_t value = 9000;
    int16_t min = 8000;
    int16_t max = 8999;
    BOOST_CHECK_THROW(ConfigFile::checkRange(value, min, max, "key", "section"), ConfigFile::Error);
  }

  {
    int32_t value = -8000;
    int32_t min = -7999;
    int32_t max = 1000;
    BOOST_CHECK_THROW(ConfigFile::checkRange(value, min, max, "key", "section"), ConfigFile::Error);
  }

  {
    int64_t value = 0x1001;
    int64_t min = -0x7fff;
    int64_t max = 0x1000;
    BOOST_CHECK_THROW(ConfigFile::checkRange(value, min, max, "key", "section"), ConfigFile::Error);
  }
}

BOOST_AUTO_TEST_SUITE_END() // TestConfigFile

} // namespace tests
} // namespace nfd
