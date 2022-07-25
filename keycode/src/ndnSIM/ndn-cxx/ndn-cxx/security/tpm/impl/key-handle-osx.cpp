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

#include "ndn-cxx/security/tpm/impl/key-handle-osx.hpp"
#include "ndn-cxx/security/tpm/impl/back-end-osx.hpp"

namespace ndn {
namespace security {
namespace tpm {

KeyHandleOsx::KeyHandleOsx(const KeyRefOsx& key)
  : m_key(key)
{
  if (m_key.get() == 0)
    NDN_THROW(Error("Key is not set"));
}

ConstBufferPtr
KeyHandleOsx::doSign(DigestAlgorithm digestAlgorithm, const InputBuffers& bufs) const
{
  return BackEndOsx::sign(m_key, digestAlgorithm, bufs);
}

bool
KeyHandleOsx::doVerify(DigestAlgorithm, const InputBuffers&, span<const uint8_t>) const
{
  NDN_THROW(Error("Signature verification is not supported with macOS Keychain-based TPM"));
}

ConstBufferPtr
KeyHandleOsx::doDecrypt(span<const uint8_t> cipherText) const
{
  return BackEndOsx::decrypt(m_key, cipherText);
}

ConstBufferPtr
KeyHandleOsx::doDerivePublicKey() const
{
  return BackEndOsx::derivePublicKey(m_key);
}

} // namespace tpm
} // namespace security
} // namespace ndn
