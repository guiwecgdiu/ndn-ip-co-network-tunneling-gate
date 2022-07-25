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

#include "ndn-cxx/security/validator-config/rule.hpp"
#include "ndn-cxx/security/validation-state.hpp"
#include "ndn-cxx/util/logger.hpp"

#include <boost/algorithm/string/predicate.hpp>

NDN_LOG_INIT(ndn.security.validator_config.Rule);

namespace ndn {
namespace security {
inline namespace v2 {
namespace validator_config {

Rule::Rule(const std::string& id, uint32_t pktType)
  : m_id(id)
  , m_pktType(pktType)
{
}

void
Rule::addFilter(unique_ptr<Filter> filter)
{
  m_filters.push_back(std::move(filter));
}

void
Rule::addChecker(unique_ptr<Checker> checker)
{
  m_checkers.push_back(std::move(checker));
}

bool
Rule::match(uint32_t pktType, const Name& pktName, const shared_ptr<ValidationState>& state) const
{
  NDN_LOG_TRACE("Trying to match " << pktName);
  if (pktType != m_pktType) {
    NDN_THROW(Error("Invalid packet type supplied (" + to_string(pktType) +
                    " != " + to_string(m_pktType) + ")"));
  }

  if (m_filters.empty()) {
    return true;
  }

  bool retval = false;
  for (const auto& filter : m_filters) {
    retval |= filter->match(pktType, pktName, state);
    if (retval) {
      break;
    }
  }
  return retval;
}

bool
Rule::check(uint32_t pktType, tlv::SignatureTypeValue sigType, const Name& pktName, const Name& klName,
            const shared_ptr<ValidationState>& state) const
{
  NDN_LOG_TRACE("Trying to check " << pktName << " with KeyLocator " << klName);

  if (pktType != m_pktType) {
    NDN_THROW(Error("Invalid packet type supplied (" + to_string(pktType) +
                    " != " + to_string(m_pktType) + ")"));
  }

  std::vector<Checker::Result> checkerResults;
  checkerResults.reserve(m_checkers.size());
  for (const auto& checker : m_checkers) {
    auto result = checker->check(pktType, sigType, pktName, klName, *state);
    if (result) {
      return true;
    }
    checkerResults.push_back(std::move(result));
  }

  std::ostringstream err;
  err << "Packet " << pktName << " (KeyLocator=" << klName << ") cannot pass any checker.";
  for (size_t i = 0; i < checkerResults.size(); ++i) {
    err << "\nChecker " << i << ": " << checkerResults[i].getErrorMessage();
  }
  state->fail({ValidationError::POLICY_ERROR, err.str()});
  return false;
}

unique_ptr<Rule>
Rule::create(const ConfigSection& configSection, const std::string& configFilename)
{
  auto propertyIt = configSection.begin();

  // Get rule.id
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "id")) {
    NDN_THROW(Error("Expecting <rule.id>"));
  }

  std::string ruleId = propertyIt->second.data();
  propertyIt++;

  // Get rule.for
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "for")) {
    NDN_THROW(Error("Expecting <rule.for> in rule: " + ruleId));
  }

  std::string usage = propertyIt->second.data();
  propertyIt++;

  bool isForData = false;
  if (boost::iequals(usage, "data")) {
    isForData = true;
  }
  else if (boost::iequals(usage, "interest")) {
    isForData = false;
  }
  else {
    NDN_THROW(Error("Unrecognized <rule.for>: " + usage + " in rule: " + ruleId));
  }

  auto rule = make_unique<Rule>(ruleId, isForData ? tlv::Data : tlv::Interest);

  // Get rule.filter(s)
  for (; propertyIt != configSection.end(); propertyIt++) {
    if (!boost::iequals(propertyIt->first, "filter")) {
      if (boost::iequals(propertyIt->first, "checker")) {
        break;
      }
      NDN_THROW(Error("Expecting <rule.filter> in rule: " + ruleId));
    }

    rule->addFilter(Filter::create(propertyIt->second, configFilename));
  }

  // Get rule.checker(s)
  bool hasCheckers = false;
  for (; propertyIt != configSection.end(); propertyIt++) {
    if (!boost::iequals(propertyIt->first, "checker")) {
      NDN_THROW(Error("Expecting <rule.checker> in rule: " + ruleId));
    }

    rule->addChecker(Checker::create(propertyIt->second, configFilename));
    hasCheckers = true;
  }

  if (propertyIt != configSection.end()) {
    NDN_THROW(Error("Expecting end of <rule>: " + ruleId));
  }

  if (!hasCheckers) {
    NDN_THROW(Error("No <rule.checker> is specified in rule: " + ruleId));
  }

  return rule;
}

} // namespace validator_config
} // inline namespace v2
} // namespace security
} // namespace ndn
