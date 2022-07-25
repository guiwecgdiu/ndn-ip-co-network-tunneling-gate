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

#include "ndn-cxx/security/validation-policy-command-interest.hpp"

#include "ndn-cxx/security/interest-signer.hpp"
#include "ndn-cxx/security/validation-policy-accept-all.hpp"
#include "ndn-cxx/security/validation-policy-simple-hierarchy.hpp"

#include "tests/test-common.hpp"
#include "tests/unit/security/validator-fixture.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/mpl/vector.hpp>

namespace ndn {
namespace security {
inline namespace v2 {
namespace tests {

using namespace ndn::tests;

BOOST_AUTO_TEST_SUITE(Security)

class CommandInterestDefaultOptions
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    return {};
  }
};

template<class T, class InnerPolicy>
class CommandInterestPolicyWrapper : public ValidationPolicyCommandInterest
{
public:
  CommandInterestPolicyWrapper()
    : ValidationPolicyCommandInterest(make_unique<InnerPolicy>(), T::getOptions())
  {
  }
};

template<class T, class InnerPolicy = ValidationPolicySimpleHierarchy>
class ValidationPolicyCommandInterestFixture : public HierarchicalValidatorFixture<CommandInterestPolicyWrapper<T, InnerPolicy>>
{
public:
  Interest
  makeCommandInterest(const Identity& identity, bool wantV3 = false)
  {
    if (wantV3) {
      Interest i(Name(identity.getName()).append("CMD"));
      m_signer.makeSignedInterest(i, signingByIdentity(identity));
      return i;
    }
    else {
      return m_signer.makeCommandInterest(Name(identity.getName()).append("CMD"),
                                          signingByIdentity(identity));
    }
  }

public:
  InterestSigner m_signer{this->m_keyChain};
};

BOOST_FIXTURE_TEST_SUITE(TestValidationPolicyCommandInterest,
                         ValidationPolicyCommandInterestFixture<CommandInterestDefaultOptions>)

BOOST_AUTO_TEST_SUITE(Accepts)

BOOST_AUTO_TEST_CASE(Basic)
{
  auto i1 = makeCommandInterest(identity);
  VALIDATE_SUCCESS(i1, "Should succeed (within grace period)");
  VALIDATE_FAILURE(i1, "Should fail (replay attack)");

  advanceClocks(5_ms);
  auto i2 = makeCommandInterest(identity);
  VALIDATE_SUCCESS(i2, "Should succeed (timestamp larger than previous)");

  auto i3 =  m_signer.makeCommandInterest(Name(identity.getName()).append("CMD"), signingWithSha256());
  VALIDATE_FAILURE(i3, "Should fail (Sha256 signature violates policy)");
}

BOOST_AUTO_TEST_CASE(BasicV3)
{
  auto i1 = makeCommandInterest(identity, true);
  VALIDATE_SUCCESS(i1, "Should succeed (within grace period)");
  VALIDATE_FAILURE(i1, "Should fail (replay attack)");

  advanceClocks(5_ms);
  auto i2 = makeCommandInterest(identity, true);
  VALIDATE_SUCCESS(i2, "Should succeed (timestamp larger than previous)");

  Interest i3(Name(identity.getName()).append("CMD"));
  m_signer.makeSignedInterest(i3, signingWithSha256());
  VALIDATE_FAILURE(i3, "Should fail (Sha256 signature violates policy)");
}

BOOST_AUTO_TEST_CASE(DataPassthru)
{
  Data d1("/Security/ValidatorFixture/Sub1");
  m_keyChain.sign(d1);
  VALIDATE_SUCCESS(d1, "Should succeed (fallback on inner validation policy for data)");
}

using ValidationPolicyAcceptAllCommands = ValidationPolicyCommandInterestFixture<CommandInterestDefaultOptions,
                                                                                 ValidationPolicyAcceptAll>;

BOOST_FIXTURE_TEST_CASE(SignedWithSha256, ValidationPolicyAcceptAllCommands) // Bug 4635
{
  auto i1 = m_signer.makeCommandInterest("/hello/world/CMD", signingWithSha256());
  VALIDATE_SUCCESS(i1, "Should succeed (within grace period)");
  VALIDATE_FAILURE(i1, "Should fail (replay attack)");

  advanceClocks(5_ms);
  auto i2 = m_signer.makeCommandInterest("/hello/world/CMD", signingWithSha256());
  VALIDATE_SUCCESS(i2, "Should succeed (timestamp larger than previous)");
}

BOOST_AUTO_TEST_SUITE_END() // Accepts

BOOST_AUTO_TEST_SUITE(Rejects)

BOOST_AUTO_TEST_CASE(NameTooShort)
{
  auto i1 = makeInterest("/name/too/short");
  VALIDATE_FAILURE(*i1, "Should fail (name is too short)");
}

BOOST_AUTO_TEST_CASE(BadTimestamp)
{
  auto i1 = makeCommandInterest(identity);
  setNameComponent(i1, command_interest::POS_TIMESTAMP, "not-timestamp");
  VALIDATE_FAILURE(i1, "Should fail (timestamp is missing)");
}

BOOST_AUTO_TEST_CASE(BadSigInfo)
{
  auto i1 = makeCommandInterest(identity);
  setNameComponent(i1, command_interest::POS_SIG_INFO, "not-SignatureInfo");
  VALIDATE_FAILURE(i1, "Should fail (signature info is missing)");
}

BOOST_AUTO_TEST_CASE(MissingKeyLocator)
{
  auto i1 = makeCommandInterest(identity);
  SignatureInfo sigInfo(tlv::SignatureSha256WithRsa);
  setNameComponent(i1, command_interest::POS_SIG_INFO,
                   sigInfo.wireEncode().begin(), sigInfo.wireEncode().end());
  VALIDATE_FAILURE(i1, "Should fail (missing KeyLocator)");
}

BOOST_AUTO_TEST_CASE(BadKeyLocatorType)
{
  auto i1 = makeCommandInterest(identity);
  KeyLocator kl;
  kl.setKeyDigest(makeBinaryBlock(tlv::KeyDigest, {0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD}));
  SignatureInfo sigInfo(tlv::SignatureSha256WithRsa);
  sigInfo.setKeyLocator(kl);
  setNameComponent(i1, command_interest::POS_SIG_INFO,
                   sigInfo.wireEncode().begin(), sigInfo.wireEncode().end());
  VALIDATE_FAILURE(i1, "Should fail (bad KeyLocator type)");
}

BOOST_AUTO_TEST_CASE(BadCertName)
{
  auto i1 = makeCommandInterest(identity);
  KeyLocator kl;
  kl.setName("/bad/cert/name");
  SignatureInfo sigInfo(tlv::SignatureSha256WithRsa);
  sigInfo.setKeyLocator(kl);
  setNameComponent(i1, command_interest::POS_SIG_INFO,
                   sigInfo.wireEncode().begin(), sigInfo.wireEncode().end());
  VALIDATE_FAILURE(i1, "Should fail (bad certificate name)");
}

BOOST_AUTO_TEST_CASE(InnerPolicyReject)
{
  auto i1 = makeCommandInterest(otherIdentity);
  VALIDATE_FAILURE(i1, "Should fail (inner policy should reject)");
}

class GracePeriod15Sec
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    ValidationPolicyCommandInterest::Options options;
    options.gracePeriod = 15_s;
    return options;
  }
};

BOOST_FIXTURE_TEST_CASE(TimestampOutOfGracePositive, ValidationPolicyCommandInterestFixture<GracePeriod15Sec>)
{
  auto i1 = makeCommandInterest(identity); // signed at 0s
  advanceClocks(16_s); // verifying at +16s
  VALIDATE_FAILURE(i1, "Should fail (timestamp outside the grace period)");
  rewindClockAfterValidation();

  auto i2 = makeCommandInterest(identity); // signed at +16s
  VALIDATE_SUCCESS(i2, "Should succeed");
}

BOOST_FIXTURE_TEST_CASE(TimestampOutOfGraceNegative, ValidationPolicyCommandInterestFixture<GracePeriod15Sec>)
{
  auto i1 = makeCommandInterest(identity); // signed at 0s
  advanceClocks(1_s);
  auto i2 = makeCommandInterest(identity); // signed at +1s
  advanceClocks(1_s);
  auto i3 = makeCommandInterest(identity); // signed at +2s

  m_systemClock->advance(-18_s); // verifying at -16s
  VALIDATE_FAILURE(i1, "Should fail (timestamp outside the grace period)");
  rewindClockAfterValidation();

  // CommandInterestValidator should not remember i1's timestamp
  VALIDATE_FAILURE(i2, "Should fail (timestamp outside the grace period)");
  rewindClockAfterValidation();

  // CommandInterestValidator should not remember i2's timestamp, and should treat i3 as initial
  advanceClocks(18_s); // verifying at +2s
  VALIDATE_SUCCESS(i3, "Should succeed");
}

BOOST_AUTO_TEST_CASE(TimestampReorderEqual)
{
  auto i1 = makeCommandInterest(identity); // signed at 0s
  VALIDATE_SUCCESS(i1, "Should succeed");

  auto i2 = makeCommandInterest(identity); // signed at 0s
  setNameComponent(i2, command_interest::POS_TIMESTAMP,
                   i1.getName()[command_interest::POS_TIMESTAMP]);
  VALIDATE_FAILURE(i2, "Should fail (timestamp reordered)");

  advanceClocks(2_s);
  auto i3 = makeCommandInterest(identity); // signed at +2s
  VALIDATE_SUCCESS(i3, "Should succeed");
}

BOOST_AUTO_TEST_CASE(TimestampReorderNegative)
{
  auto i2 = makeCommandInterest(identity); // signed at 0ms
  advanceClocks(200_ms);
  auto i3 = makeCommandInterest(identity); // signed at +200ms
  advanceClocks(900_ms);
  auto i1 = makeCommandInterest(identity); // signed at +1100ms
  advanceClocks(300_ms);
  auto i4 = makeCommandInterest(identity); // signed at +1400ms

  m_systemClock->advance(-300_ms); // verifying at +1100ms
  VALIDATE_SUCCESS(i1, "Should succeed");
  rewindClockAfterValidation();

  m_systemClock->advance(-1100_ms); // verifying at 0ms
  VALIDATE_FAILURE(i2, "Should fail (timestamp reordered)");
  rewindClockAfterValidation();

  // CommandInterestValidator should not remember i2's timestamp
  advanceClocks(200_ms); // verifying at +200ms
  VALIDATE_FAILURE(i3, "Should fail (timestamp reordered)");
  rewindClockAfterValidation();

  advanceClocks(1200_ms); // verifying at 1400ms
  VALIDATE_SUCCESS(i4, "Should succeed");
}

BOOST_AUTO_TEST_SUITE_END() // Rejects

BOOST_AUTO_TEST_SUITE(Options)

template<class T>
class GracePeriod
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    ValidationPolicyCommandInterest::Options options;
    options.gracePeriod = time::seconds(T::value);
    return options;
  }
};

typedef boost::mpl::vector<
  GracePeriod<boost::mpl::int_<0>>,
  GracePeriod<boost::mpl::int_<-1>>
> GraceNonPositiveValues;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(GraceNonPositive, GracePeriod, GraceNonPositiveValues,
                                 ValidationPolicyCommandInterestFixture<GracePeriod>)
{
  auto i1 = this->makeCommandInterest(this->identity); // signed at 0ms
  auto i2 = this->makeCommandInterest(this->subIdentity); // signed at 0ms
  for (auto interest : {&i1, &i2}) {
    setNameComponent(*interest, command_interest::POS_TIMESTAMP,
                     name::Component::fromNumber(time::toUnixTimestamp(time::system_clock::now()).count()));
  } // ensure timestamps are exactly 0ms

  VALIDATE_SUCCESS(i1, "Should succeed when validating at 0ms");
  this->rewindClockAfterValidation();

  this->advanceClocks(1_ms);
  VALIDATE_FAILURE(i2, "Should fail when validating at 1ms");
}

class LimitedRecordsOptions
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    ValidationPolicyCommandInterest::Options options;
    options.gracePeriod = 15_s;
    options.maxRecords = 3;
    return options;
  }
};

BOOST_FIXTURE_TEST_CASE(LimitedRecords, ValidationPolicyCommandInterestFixture<LimitedRecordsOptions>)
{
  Identity id1 = this->addSubCertificate("/Security/ValidatorFixture/Sub1", identity);
  this->cache.insert(id1.getDefaultKey().getDefaultCertificate());
  Identity id2 = this->addSubCertificate("/Security/ValidatorFixture/Sub2", identity);
  this->cache.insert(id2.getDefaultKey().getDefaultCertificate());
  Identity id3 = this->addSubCertificate("/Security/ValidatorFixture/Sub3", identity);
  this->cache.insert(id3.getDefaultKey().getDefaultCertificate());
  Identity id4 = this->addSubCertificate("/Security/ValidatorFixture/Sub4", identity);
  this->cache.insert(id4.getDefaultKey().getDefaultCertificate());

  auto i1 = makeCommandInterest(id2);
  auto i2 = makeCommandInterest(id3);
  auto i3 = makeCommandInterest(id4);
  auto i00 = makeCommandInterest(id1); // signed at 0s
  advanceClocks(1_s);
  auto i01 = makeCommandInterest(id1); // signed at 1s
  advanceClocks(1_s);
  auto i02 = makeCommandInterest(id1); // signed at 2s

  VALIDATE_SUCCESS(i00, "Should succeed");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i02, "Should succeed");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i1, "Should succeed");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i2, "Should succeed");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i3, "Should succeed, forgets identity id1");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i01, "Should succeed despite timestamp is reordered, because record has been evicted");
}

class UnlimitedRecordsOptions
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    ValidationPolicyCommandInterest::Options options;
    options.gracePeriod = 15_s;
    options.maxRecords = -1;
    return options;
  }
};

BOOST_FIXTURE_TEST_CASE(UnlimitedRecords, ValidationPolicyCommandInterestFixture<UnlimitedRecordsOptions>)
{
  std::vector<Identity> identities;
  for (size_t i = 0; i < 20; ++i) {
    Identity id = this->addSubCertificate("/Security/ValidatorFixture/Sub" + to_string(i), identity);
    this->cache.insert(id.getDefaultKey().getDefaultCertificate());
    identities.push_back(id);
  }

  auto i1 = makeCommandInterest(identities.at(0)); // signed at 0s
  advanceClocks(1_s);
  for (size_t i = 0; i < 20; ++i) {
    auto i2 = makeCommandInterest(identities.at(i)); // signed at +1s

    VALIDATE_SUCCESS(i2, "Should succeed");
    rewindClockAfterValidation();
  }
  VALIDATE_FAILURE(i1, "Should fail (timestamp reorder)");
}

class ZeroRecordsOptions
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    ValidationPolicyCommandInterest::Options options;
    options.gracePeriod = 15_s;
    options.maxRecords = 0;
    return options;
  }
};

BOOST_FIXTURE_TEST_CASE(ZeroRecords, ValidationPolicyCommandInterestFixture<ZeroRecordsOptions>)
{
  auto i1 = makeCommandInterest(identity); // signed at 0s
  advanceClocks(1_s);
  auto i2 = makeCommandInterest(identity); // signed at +1s
  VALIDATE_SUCCESS(i2, "Should succeed");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i1, "Should succeed despite timestamp is reordered, because record isn't kept");
}

class LimitedRecordLifetimeOptions
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    ValidationPolicyCommandInterest::Options options;
    options.gracePeriod = 400_s;
    options.recordLifetime = 300_s;
    return options;
  }
};

BOOST_FIXTURE_TEST_CASE(LimitedRecordLifetime, ValidationPolicyCommandInterestFixture<LimitedRecordLifetimeOptions>)
{
  auto i1 = makeCommandInterest(identity); // signed at 0s
  advanceClocks(240_s);
  auto i2 = makeCommandInterest(identity); // signed at +240s
  advanceClocks(120_s);
  auto i3 = makeCommandInterest(identity); // signed at +360s

  m_systemClock->advance(-360_s); // rewind system clock to 0s
  VALIDATE_SUCCESS(i1, "Should succeed");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i3, "Should succeed");
  rewindClockAfterValidation();

  advanceClocks(30_s, 301_s); // advance steady clock by 301s, and system clock to +301s
  VALIDATE_SUCCESS(i2, "Should succeed despite timestamp is reordered, because record has been expired");
}

class ZeroRecordLifetimeOptions
{
public:
  static ValidationPolicyCommandInterest::Options
  getOptions()
  {
    ValidationPolicyCommandInterest::Options options;
    options.gracePeriod = 15_s;
    options.recordLifetime = time::seconds::zero();
    return options;
  }
};

BOOST_FIXTURE_TEST_CASE(ZeroRecordLifetime, ValidationPolicyCommandInterestFixture<ZeroRecordLifetimeOptions>)
{
  auto i1 = makeCommandInterest(identity); // signed at 0s
  advanceClocks(1_s);
  auto i2 = makeCommandInterest(identity); // signed at +1s
  VALIDATE_SUCCESS(i2, "Should succeed");
  rewindClockAfterValidation();

  VALIDATE_SUCCESS(i1, "Should succeed despite timestamp is reordered, because record has been expired");
}

BOOST_AUTO_TEST_SUITE_END() // Options

BOOST_AUTO_TEST_SUITE_END() // TestValidationPolicyCommandInterest
BOOST_AUTO_TEST_SUITE_END() // Security

} // namespace tests
} // inline namespace v2
} // namespace security
} // namespace ndn
