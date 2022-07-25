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

#ifndef NDN_CXX_SECURITY_TRANSFORM_VERIFIER_FILTER_HPP
#define NDN_CXX_SECURITY_TRANSFORM_VERIFIER_FILTER_HPP

#include "ndn-cxx/security/transform/transform-base.hpp"
#include "ndn-cxx/security/security-common.hpp"

namespace ndn {
namespace security {
namespace transform {

class PrivateKey;
class PublicKey;

/**
 * @brief The module to verify signatures.
 *
 * The next module in the chain is usually BoolSink.
 */
class VerifierFilter final : public Transform
{
public:
  /**
   * @brief Create a verifier module to verify signature @p sig using algorithm @p algo and public key @p key
   */
  VerifierFilter(DigestAlgorithm algo, const PublicKey& key, span<const uint8_t> sig);

  /**
   * @brief Create a verifier module to verify signature @p sig using algorithm @p algo and HMAC key @p key
   */
  VerifierFilter(DigestAlgorithm algo, const PrivateKey& key, span<const uint8_t> sig);

  ~VerifierFilter() final;

private:
  void
  init(DigestAlgorithm algo, void* pkey);

  /**
   * @brief Write data @p buf into verifier
   *
   * @return The number of bytes that are actually written
   */
  size_t
  convert(span<const uint8_t> buf) final;

  /**
   * @brief Finalize verification and write the result (single byte) into next module.
   */
  void
  finalize() final;

private:
  class Impl;
  const unique_ptr<Impl> m_impl;

  KeyType m_keyType;
};

unique_ptr<Transform>
verifierFilter(DigestAlgorithm algo, const PublicKey& key, span<const uint8_t> sig);

unique_ptr<Transform>
verifierFilter(DigestAlgorithm algo, const PrivateKey& key, span<const uint8_t> sig);

} // namespace transform
} // namespace security
} // namespace ndn

#endif // NDN_CXX_SECURITY_TRANSFORM_VERIFIER_FILTER_HPP
