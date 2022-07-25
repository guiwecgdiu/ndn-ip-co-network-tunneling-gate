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

#ifndef NDN_CXX_SECURITY_CERTIFICATE_REQUEST_HPP
#define NDN_CXX_SECURITY_CERTIFICATE_REQUEST_HPP

#include "ndn-cxx/interest.hpp"

namespace ndn {
namespace security {
inline namespace v2 {

/**
 * @brief Request for a certificate, associated with the number of attempts
 */
class CertificateRequest : noncopyable
{
public:
  CertificateRequest() = default;

  explicit
  CertificateRequest(const Interest& interest)
    : interest(interest)
    , nRetriesLeft(3)
  {
  }

  explicit
  CertificateRequest(const Name& name)
    : CertificateRequest(Interest(name).setCanBePrefix(true))
  {
  }

public:
  /// @brief the name for the requested data/certificate.
  Interest interest;
  /// @brief the number of remaining retries after timeout or NACK.
  int nRetriesLeft = 0;
  /// @brief the amount of time to wait before sending the next interest after a NACK.
  time::milliseconds waitAfterNack = 500_ms;
};

} // inline namespace v2
} // namespace security
} // namespace ndn

#endif // NDN_CXX_SECURITY_CERTIFICATE_REQUEST_HPP
