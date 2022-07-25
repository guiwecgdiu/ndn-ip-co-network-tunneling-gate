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
 *
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

#ifndef NDN_CXX_ENCODING_BUFFER_HPP
#define NDN_CXX_ENCODING_BUFFER_HPP

#include "ndn-cxx/detail/common.hpp"

#include <initializer_list>
#include <vector>

namespace ndn {

/**
 * @brief General-purpose automatically managed/resized buffer
 *
 * In most respect, the Buffer class is equivalent to a `std::vector<uint8_t>`, and it in fact
 * uses the latter as a base class. In addition to that, it provides the get<T>() helper method
 * that automatically casts the returned pointer to the requested type.
 */
class Buffer : public std::vector<uint8_t>
{
public:
  /** @brief Creates an empty Buffer
   */
  Buffer() = default;

  /** @brief Copy constructor
   */
  Buffer(const Buffer&);

  /** @brief Copy assignment operator
   */
  Buffer&
  operator=(const Buffer&);

  /** @brief Move constructor
   */
  Buffer(Buffer&&) noexcept;

  /** @brief Move assignment operator
   */
  Buffer&
  operator=(Buffer&&) noexcept;

  /** @brief Creates a Buffer with pre-allocated size
   *  @param size size of the Buffer to be allocated
   */
  explicit
  Buffer(size_t size)
    : std::vector<uint8_t>(size, 0)
  {
  }

  /** @brief Creates a Buffer by copying contents from a raw buffer
   *  @param buf const pointer to buffer to copy
   *  @param length length of the buffer to copy
   */
  Buffer(const void* buf, size_t length)
    : std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(buf),
                           reinterpret_cast<const uint8_t*>(buf) + length)
  {
  }

  /** @brief Creates a Buffer by copying the elements of the range [first, last)
   *  @param first an input iterator to the first element to copy
   *  @param last an input iterator to the element immediately following the last element to copy
   */
  template<class InputIt>
  Buffer(InputIt first, InputIt last)
    : std::vector<uint8_t>(first, last)
  {
  }

  /** @brief Creates a Buffer with the contents of an initializer list
   */
  Buffer(std::initializer_list<uint8_t> il)
    : std::vector<uint8_t>(il)
  {
  }

  /** @return pointer to the first byte of the buffer, cast to the requested type T
   */
  template<class T>
  T*
  get() noexcept
  {
    return reinterpret_cast<T*>(data());
  }

  /** @return const pointer to the first byte of the buffer, cast to the requested type T
   */
  template<class T>
  const T*
  get() const noexcept
  {
    return reinterpret_cast<const T*>(data());
  }
};

inline
Buffer::Buffer(const Buffer&) = default;

inline Buffer&
Buffer::operator=(const Buffer&) = default;

inline
Buffer::Buffer(Buffer&&) noexcept = default;

inline Buffer&
Buffer::operator=(Buffer&&) noexcept = default;

/** \cond */
std::ostream&
boost_test_print_type(std::ostream&, const Buffer&);
/** \endcond */

using BufferPtr = shared_ptr<Buffer>;
using ConstBufferPtr = shared_ptr<const Buffer>;

} // namespace ndn

#endif // NDN_CXX_ENCODING_BUFFER_HPP
