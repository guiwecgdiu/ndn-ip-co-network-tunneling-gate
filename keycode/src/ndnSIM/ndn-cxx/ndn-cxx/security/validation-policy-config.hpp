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

#ifndef NDN_CXX_SECURITY_VALIDATION_POLICY_CONFIG_HPP
#define NDN_CXX_SECURITY_VALIDATION_POLICY_CONFIG_HPP

#include "ndn-cxx/security/validation-policy.hpp"
#include "ndn-cxx/security/validator-config/rule.hpp"

namespace ndn {
namespace security {
inline namespace v2 {
namespace validator_config {

/**
 * @brief A validator that can be set up via a configuration file.
 *
 * @note For command Interest validation, this policy must be combined with
 *       @p ValidationPolicyCommandInterest, in order to guard against replay attacks.
 * @note This policy does not support inner policies (a sole policy or a terminal inner policy)
 * @sa https://named-data.net/doc/ndn-cxx/current/tutorials/security-validator-config.html
 */
class ValidationPolicyConfig : public ValidationPolicy
{
public:
  /**
   * @brief Load policy from file @p filename
   * @throw Error Validator instance not assigned to the policy (m_validator == nullptr)
   */
  void
  load(const std::string& filename);

  /**
   * @brief Load policy from direct @p input
   * @throw Error Validator instance not assigned to the policy (m_validator == nullptr)
   */
  void
  load(const std::string& input, const std::string& filename);

  /**
   * @brief Load policy from direct @p input
   * @throw Error Validator instance not assigned to the policy (m_validator == nullptr)
   */
  void
  load(std::istream& input, const std::string& filename);

  /**
   * @brief Load policy from @p configSection
   * @throw Error Validator instance not assigned to the policy (m_validator == nullptr)
   */
  void
  load(const ConfigSection& configSection, const std::string& filename);

protected:
  void
  checkPolicy(const Data& data, const shared_ptr<ValidationState>& state,
              const ValidationContinuation& continueValidation) override;

  void
  checkPolicy(const Interest& interest, const shared_ptr<ValidationState>& state,
              const ValidationContinuation& continueValidation) override;

private:
  void
  processConfigTrustAnchor(const ConfigSection& section, const std::string& filename);

  time::nanoseconds
  getRefreshPeriod(ConfigSection::const_iterator& it, const ConfigSection::const_iterator& end);

  time::nanoseconds
  getDefaultRefreshPeriod();

NDN_CXX_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /** @brief Whether to always bypass validation.
   *
   *  This is set to true when 'any' is specified as a trust anchor.
   *  It causes all packets to bypass validation.
   */
  bool m_shouldBypass = false;
  bool m_isConfigured = false;

  std::vector<unique_ptr<Rule>> m_dataRules;
  std::vector<unique_ptr<Rule>> m_interestRules;
};

} // namespace validator_config

using validator_config::ValidationPolicyConfig;

} // inline namespace v2
} // namespace security
} // namespace ndn

#endif // NDN_CXX_SECURITY_VALIDATION_POLICY_CONFIG_HPP
