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

#include "ndn-cxx/security/validation-policy-config.hpp"
#include "ndn-cxx/security/validator.hpp"
#include "ndn-cxx/util/io.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>

#include <fstream>

namespace ndn {
namespace security {
inline namespace v2 {
namespace validator_config {

void
ValidationPolicyConfig::load(const std::string& filename)
{
  std::ifstream inputFile(filename);
  if (!inputFile) {
    NDN_THROW(Error("Failed to read configuration file: " + filename));
  }
  load(inputFile, filename);
}

void
ValidationPolicyConfig::load(const std::string& input, const std::string& filename)
{
  std::istringstream inputStream(input);
  load(inputStream, filename);
}

void
ValidationPolicyConfig::load(std::istream& input, const std::string& filename)
{
  ConfigSection tree;
  try {
    boost::property_tree::read_info(input, tree);
  }
  catch (const boost::property_tree::info_parser_error& e) {
    NDN_THROW(Error("Failed to parse configuration file " + filename +
                    " line " + to_string(e.line()) + ": " + e.message()));
  }
  load(tree, filename);
}

void
ValidationPolicyConfig::load(const ConfigSection& configSection, const std::string& filename)
{
  BOOST_ASSERT(!filename.empty());

  if (m_validator == nullptr) {
    NDN_THROW(Error("Validator instance not assigned on the policy"));
  }
  if (m_isConfigured) {
    m_shouldBypass = false;
    m_dataRules.clear();
    m_interestRules.clear();
    m_validator->resetAnchors();
    m_validator->resetVerifiedCertificates();
  }
  m_isConfigured = true;

  for (const auto& subSection : configSection) {
    const std::string& sectionName = subSection.first;
    const ConfigSection& section = subSection.second;

    if (boost::iequals(sectionName, "rule")) {
      auto rule = Rule::create(section, filename);
      if (rule->getPktType() == tlv::Data) {
        m_dataRules.push_back(std::move(rule));
      }
      else if (rule->getPktType() == tlv::Interest) {
        m_interestRules.push_back(std::move(rule));
      }
    }
    else if (boost::iequals(sectionName, "trust-anchor")) {
      processConfigTrustAnchor(section, filename);
    }
    else {
      NDN_THROW(Error("Error processing configuration file " + filename +
                      ": unrecognized section " + sectionName));
    }
  }
}

void
ValidationPolicyConfig::processConfigTrustAnchor(const ConfigSection& configSection,
                                                 const std::string& filename)
{
  using namespace boost::filesystem;

  auto propertyIt = configSection.begin();

  // Get trust-anchor.type
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "type")) {
    NDN_THROW(Error("Expecting <trust-anchor.type>"));
  }

  std::string type = propertyIt->second.data();
  propertyIt++;

  if (boost::iequals(type, "file")) {
    // Get trust-anchor.file
    if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "file-name")) {
      NDN_THROW(Error("Expecting <trust-anchor.file-name>"));
    }

    std::string file = propertyIt->second.data();
    propertyIt++;

    time::nanoseconds refresh = getRefreshPeriod(propertyIt, configSection.end());
    if (propertyIt != configSection.end())
      NDN_THROW(Error("Expecting end of <trust-anchor>"));

    m_validator->loadAnchor(file, absolute(file, path(filename).parent_path()).string(),
                            refresh, false);
  }
  else if (boost::iequals(type, "base64")) {
    // Get trust-anchor.base64-string
    if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "base64-string"))
      NDN_THROW(Error("Expecting <trust-anchor.base64-string>"));

    std::stringstream ss(propertyIt->second.data());
    propertyIt++;

    if (propertyIt != configSection.end())
      NDN_THROW(Error("Expecting end of <trust-anchor>"));

    auto idCert = io::load<Certificate>(ss);
    if (idCert != nullptr) {
      m_validator->loadAnchor("", std::move(*idCert));
    }
    else {
      NDN_THROW(Error("Cannot decode certificate from base64-string"));
    }
  }
  else if (boost::iequals(type, "dir")) {
    if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "dir"))
      NDN_THROW(Error("Expecting <trust-anchor.dir>"));

    std::string dirString(propertyIt->second.data());
    propertyIt++;

    time::nanoseconds refresh = getRefreshPeriod(propertyIt, configSection.end());
    if (propertyIt != configSection.end())
      NDN_THROW(Error("Expecting end of <trust-anchor>"));

    path dirPath = absolute(dirString, path(filename).parent_path());
    m_validator->loadAnchor(dirString, dirPath.string(), refresh, true);
  }
  else if (boost::iequals(type, "any")) {
    m_shouldBypass = true;
  }
  else {
    NDN_THROW(Error("Unrecognized <trust-anchor.type>: " + type));
  }
}

time::nanoseconds
ValidationPolicyConfig::getRefreshPeriod(ConfigSection::const_iterator& it,
                                         const ConfigSection::const_iterator& end)
{
  auto refresh = time::nanoseconds::max();
  if (it == end) {
    return refresh;
  }

  if (!boost::iequals(it->first, "refresh")) {
    NDN_THROW(Error("Expecting <trust-anchor.refresh>"));
  }

  std::string inputString = it->second.data();
  ++it;
  char unit = inputString[inputString.size() - 1];
  std::string refreshString = inputString.substr(0, inputString.size() - 1);

  int32_t refreshPeriod = -1;
  try {
    refreshPeriod = boost::lexical_cast<int32_t>(refreshString);
  }
  catch (const boost::bad_lexical_cast&) {
    // pass
  }
  if (refreshPeriod < 0) {
    NDN_THROW(Error("Bad refresh value: " + refreshString));
  }

  if (refreshPeriod == 0) {
    return getDefaultRefreshPeriod();
  }

  switch (unit) {
    case 'h':
      return time::hours(refreshPeriod);
    case 'm':
      return time::minutes(refreshPeriod);
    case 's':
      return time::seconds(refreshPeriod);
    default:
      NDN_THROW(Error("Bad refresh time unit: "s + unit));
  }
}

time::nanoseconds
ValidationPolicyConfig::getDefaultRefreshPeriod()
{
  return 1_h;
}

void
ValidationPolicyConfig::checkPolicy(const Data& data, const shared_ptr<ValidationState>& state,
                                    const ValidationContinuation& continueValidation)
{
  BOOST_ASSERT_MSG(!hasInnerPolicy(), "ValidationPolicyConfig must be a terminal inner policy");

  if (m_shouldBypass) {
    return continueValidation(nullptr, state);
  }

  Name klName = getKeyLocatorName(data, *state);
  if (!state->getOutcome()) { // already failed
    return;
  }

  for (const auto& rule : m_dataRules) {
    if (rule->match(tlv::Data, data.getName(), state)) {
      if (rule->check(tlv::Data, tlv::SignatureTypeValue(data.getSignatureType()),
                      data.getName(), klName, state)) {
        return continueValidation(make_shared<CertificateRequest>(klName), state);
      }
      // rule->check calls state->fail(...) if the check fails
      return;
    }
  }

  return state->fail({ValidationError::POLICY_ERROR,
                      "No rule matched for data `" + data.getName().toUri() + "`"});
}

void
ValidationPolicyConfig::checkPolicy(const Interest& interest, const shared_ptr<ValidationState>& state,
                                    const ValidationContinuation& continueValidation)
{
  BOOST_ASSERT_MSG(!hasInnerPolicy(), "ValidationPolicyConfig must be a terminal inner policy");

  if (m_shouldBypass) {
    return continueValidation(nullptr, state);
  }

  Name klName = getKeyLocatorName(interest, *state);
  if (!state->getOutcome()) { // already failed
    return;
  }

  for (const auto& rule : m_interestRules) {
    if (rule->match(tlv::Interest, interest.getName(), state)) {

      tlv::SignatureTypeValue sigType;
      auto fmt = state->getTag<SignedInterestFormatTag>();
      BOOST_ASSERT(fmt);

      if (*fmt == SignedInterestFormat::V03) {
        sigType = tlv::SignatureTypeValue(interest.getSignatureInfo()->getSignatureType());
      }
      else {
        if (interest.getName().size() < signed_interest::MIN_SIZE) {
          state->fail({ValidationError::INVALID_KEY_LOCATOR, "Invalid signed Interest: name too short"});
          return;
        }

        SignatureInfo si;
        try {
          si.wireDecode(interest.getName().at(signed_interest::POS_SIG_INFO).blockFromValue());
        }
        catch (const tlv::Error& e) {
          state->fail({ValidationError::Code::INVALID_KEY_LOCATOR,
                       "Invalid signed Interest: " + std::string(e.what())});
          return;
        }

        sigType = tlv::SignatureTypeValue(si.getSignatureType());
      }

      if (rule->check(tlv::Interest, sigType, interest.getName(), klName, state)) {
        return continueValidation(make_shared<CertificateRequest>(klName), state);
      }
      // rule->check calls state->fail(...) if the check fails
      return;
    }
  }

  return state->fail({ValidationError::POLICY_ERROR,
                      "No rule matched for interest `" + interest.getName().toUri() + "`"});
}

} // namespace validator_config
} // inline namespace v2
} // namespace security
} // namespace ndn
