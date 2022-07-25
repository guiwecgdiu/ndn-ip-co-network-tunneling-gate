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

#ifndef NDN_CXX_UTIL_RANDOM_HPP
#define NDN_CXX_UTIL_RANDOM_HPP

#include "ndn-cxx/detail/common.hpp"
#include "ndn-cxx/util/span.hpp"

#include <random>

namespace ndn {
namespace random {

/**
 * @brief Generate a cryptographically secure random integer from the range [0, 2^32)
 *
 * @throw std::runtime_error if generation fails.
 */
uint32_t
generateSecureWord32();

/**
 * @brief Generate a cryptographically secure random integer from the range [0, 2^64)
 *
 * @throw std::runtime_error if generation fails.
 */
uint64_t
generateSecureWord64();

/**
 * @brief Fill @p buffer with cryptographically secure random bytes
 *
 * @throw std::runtime_error if generation fails.
 */
void
generateSecureBytes(span<uint8_t> buffer);

using RandomNumberEngine = std::mt19937;

/**
 * @brief Returns a reference to a thread-local instance of a properly seeded PRNG
 *
 * @warning The returned RandomNumberEngine MUST NOT be used when cryptographically secure
 *          random numbers are needed.
 */
RandomNumberEngine&
getRandomNumberEngine();

/**
 * @brief Generate a non-cryptographically-secure random integer in the range [0, 2^32)
 *
 * This version is faster than generateSecureWord32, but it must not be used when
 * cryptographically secure random integers are needed (e.g., when creating signing or
 * encryption keys)
 */
uint32_t
generateWord32();

/**
 * @brief Generate a non-cryptographically-secure random integer in the range [0, 2^64)
 *
 * This version is faster than generateSecureWord64, but it must not be used when
 * cryptographically secure random integers are needed (e.g., when creating signing or
 * encryption keys)
 */
uint64_t
generateWord64();

} // namespace random
} // namespace ndn

#endif // NDN_CXX_UTIL_RANDOM_HPP
