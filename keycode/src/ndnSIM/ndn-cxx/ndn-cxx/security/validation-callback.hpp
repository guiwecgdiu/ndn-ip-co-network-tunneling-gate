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

#ifndef NDN_CXX_SECURITY_VALIDATION_CALLBACK_HPP
#define NDN_CXX_SECURITY_VALIDATION_CALLBACK_HPP

#include "ndn-cxx/data.hpp"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/security/security-common.hpp"
#include "ndn-cxx/security/validation-error.hpp"

namespace ndn {
namespace security {
inline namespace v2 {

/**
 * @brief Callback to report a successful Data validation.
 */
typedef function<void(const Data& data)> DataValidationSuccessCallback;

/**
 * @brief Callback to report a failed Data validation.
 */
typedef function<void(const Data& data, const ValidationError& error)> DataValidationFailureCallback;

/**
 * @brief Callback to report a successful Interest validation.
 */
typedef function<void(const Interest& interest)> InterestValidationSuccessCallback;

/**
 * @brief Callback to report a failed Interest validation.
 */
typedef function<void(const Interest& interest, const ValidationError& error)> InterestValidationFailureCallback;

} // inline namespace v2
} // namespace security
} // namespace ndn

#endif // NDN_CXX_SECURITY_VALIDATION_CALLBACK_HPP
