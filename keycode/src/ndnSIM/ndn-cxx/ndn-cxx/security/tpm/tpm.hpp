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

#ifndef NDN_CXX_SECURITY_TPM_TPM_HPP
#define NDN_CXX_SECURITY_TPM_TPM_HPP

#include "ndn-cxx/name.hpp"
#include "ndn-cxx/security/key-params.hpp"
#include "ndn-cxx/security/tpm/key-handle.hpp"

#include <unordered_map>
#include <boost/logic/tribool.hpp>

namespace ndn {
namespace security {

namespace transform {
class PrivateKey;
} // namespace transform

inline namespace v2 {
class KeyChain;
} // inline namespace v2

namespace tpm {

class BackEnd;

/**
 * @brief TPM front-end class.
 *
 * The TPM (Trusted Platform Module) stores the private portion of a user's cryptography keys.
 * The format and location of stored information is indicated by the TpmLocator.
 * The TPM is designed to work with a PIB (Public Information Base) which stores public keys and
 * related information such as certificate.
 *
 * The TPM also provides functionalities of crypto transformation, such as signing and decryption.
 *
 * A TPM consists of a unified front-end interface and a back-end implementation. The front-end
 * cache the handles of private keys which is provided by the back-end implementation.
 *
 * @note Tpm instance is created and managed only by KeyChain. KeyChain::getTpm() returns
 *       a const reference to the managed Tpm instance, through which it is possible to
 *       check the existence of private keys, get the public key corresponding to a private
 *       key, sign, and decrypt the supplied buffers using managed private keys.
 */
class Tpm : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  ~Tpm();

  std::string
  getTpmLocator() const;

  /**
   * @brief Check if a private key exists.
   *
   * @param keyName The key name
   * @return true if the key exists
   */
  bool
  hasKey(const Name& keyName) const;

  /**
   * @return The public portion of an asymmetric key with name @p keyName,
   *         or nullptr if the key does not exist,
   *
   * The public key is in PKCS#8 format.
   */
  ConstBufferPtr
  getPublicKey(const Name& keyName) const;

  /**
   * @brief Sign discontiguous ranges using the key with name @p keyName and using the digest
   *        @p digestAlgorithm.
   *
   * @return The signature, or nullptr if the key does not exist.
   */
  ConstBufferPtr
  sign(const InputBuffers& bufs, const Name& keyName, DigestAlgorithm digestAlgorithm) const;

  /**
   * @brief Verify discontiguous ranges using the key with name @p keyName and using the digest
   *        @p digestAlgorithm.
   *
   * @retval true the signature is valid
   * @retval false the signature is not valid
   * @retval indeterminate the key does not exist
   */
  NDN_CXX_NODISCARD boost::logic::tribool
  verify(const InputBuffers& bufs, span<const uint8_t> sig, const Name& keyName,
         DigestAlgorithm digestAlgorithm) const;

  /**
   * @brief Decrypt blob using the key with name @p keyName.
   *
   * @return The decrypted data, or nullptr if the key does not exist.
   */
  ConstBufferPtr
  decrypt(span<const uint8_t> buf, const Name& keyName) const;

public: // Management
  /**
   * @brief Check if the TPM is in terminal mode.
   */
  bool
  isTerminalMode() const;

  /**
   * @brief Set the terminal mode of the TPM.
   *
   * When in terminal mode, the TPM will not ask user permission from GUI.
   */
  void
  setTerminalMode(bool isTerminal) const;

  /**
   * @return true if the TPM is locked, otherwise false.
   */
  bool
  isTpmLocked() const;

  /**
   * @brief Unlock the TPM.
   *
   * @param password The password to unlock the TPM.
   * @param passwordLength The password size.
   */
  NDN_CXX_NODISCARD bool
  unlockTpm(const char* password, size_t passwordLength) const;

NDN_CXX_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   * @brief Create a new TPM instance with the specified @p location.
   *
   * @param scheme The scheme for the TPM
   * @param location The location for the TPM
   * @param impl The back-end implementation
   */
  Tpm(const std::string& scheme, const std::string& location, unique_ptr<BackEnd> impl);

  /**
   * @brief Create key for @p identityName according to @p params.
   *
   * The created key is named as follows:
   * - RSA and EC keys: `/<identityName>/KEY/<keyId>`
   * - HMAC keys: `/<identityName>/<keyDigest>`
   *
   * @return The key name.
   * @throw Error The key already exists or @p params is invalid.
   */
  Name
  createKey(const Name& identityName, const KeyParams& params);

  /**
   * @brief Delete a key pair with name @p keyName.
   */
  void
  deleteKey(const Name& keyName);

  /**
   * @brief Export a private key.
   *
   * Export a private key in encrypted PKCS #8 format.
   *
   * @param keyName The private key name
   * @param pw The password to encrypt the private key
   * @param pwLen The length of the password
   * @return The encoded private key wrapper.
   *
   * @throw Error The key does not exist or it could not be exported.
   */
  ConstBufferPtr
  exportPrivateKey(const Name& keyName, const char* pw, size_t pwLen) const;

  /**
   * @brief Import a private key.
   *
   * Import a private key in encrypted PKCS #8 format.
   *
   * @param keyName The private key name.
   * @param pkcs8 The key encoded in PKCS #8 format.
   * @param pw The password to decrypt the private key.
   * @param pwLen The length of the password.
   *
   * @throw Error The key could not be imported.
   */
  void
  importPrivateKey(const Name& keyName, span<const uint8_t> pkcs8, const char* pw, size_t pwLen);

  /**
   * @brief Import a private key.
   */
  void
  importPrivateKey(const Name& keyName, shared_ptr<transform::PrivateKey> key);

  /**
   * @brief Clear the key cache.
   *
   * An empty cache can force Tpm to do key lookup in the back-end.
   */
  void
  clearKeyCache()
  {
    m_keys.clear();
  }

private:
  /**
   * @brief Internal KeyHandle lookup.
   *
   * @return A pointer to the handle of key @p keyName if it exists, otherwise nullptr.
   */
  const KeyHandle*
  findKey(const Name& keyName) const;

private:
  std::string m_scheme;
  std::string m_location;

  mutable std::unordered_map<Name, unique_ptr<KeyHandle>> m_keys;

  const unique_ptr<BackEnd> m_backEnd;

  friend KeyChain;
};

} // namespace tpm

using tpm::Tpm;

} // namespace security
} // namespace ndn

#endif // NDN_CXX_SECURITY_TPM_TPM_HPP
