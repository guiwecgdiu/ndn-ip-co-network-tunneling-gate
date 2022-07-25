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

#ifndef NDN_CXX_ENCODING_BLOCK_HELPERS_HPP
#define NDN_CXX_ENCODING_BLOCK_HELPERS_HPP

#include "ndn-cxx/encoding/block.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ndn-cxx/util/concepts.hpp"

namespace ndn {
namespace encoding {

/** @brief Prepend a TLV element containing a non-negative integer.
 *  @param encoder an EncodingBuffer or EncodingEstimator
 *  @param type TLV-TYPE number
 *  @param value non-negative integer value
 *  @sa makeNonNegativeIntegerBlock, readNonNegativeInteger
 */
template<Tag TAG>
size_t
prependNonNegativeIntegerBlock(EncodingImpl<TAG>& encoder, uint32_t type, uint64_t value);

extern template size_t
prependNonNegativeIntegerBlock<EstimatorTag>(EncodingImpl<EstimatorTag>&, uint32_t, uint64_t);

extern template size_t
prependNonNegativeIntegerBlock<EncoderTag>(EncodingImpl<EncoderTag>&, uint32_t, uint64_t);

/** @brief Create a TLV block containing a non-negative integer.
 *  @param type TLV-TYPE number
 *  @param value non-negative integer value
 *  @sa prependNonNegativeIntegerBlock, readNonNegativeInteger
 */
Block
makeNonNegativeIntegerBlock(uint32_t type, uint64_t value);

/** @brief Read a non-negative integer from a TLV element.
 *  @param block the TLV element
 *  @throw tlv::Error block does not contain a non-negative integer
 *  @sa prependNonNegativeIntegerBlock, makeNonNegativeIntegerBlock
 */
uint64_t
readNonNegativeInteger(const Block& block);

/** @brief Read a non-negative integer from a TLV element and cast to the specified type.
 *  @tparam R result type, must be an integral type
 *  @param block the TLV element
 *  @throw tlv::Error block does not contain a valid non-negative integer or the number cannot be
 *                    represented in R
 */
template<typename R>
std::enable_if_t<std::is_integral<R>::value, R>
readNonNegativeIntegerAs(const Block& block)
{
  uint64_t value = readNonNegativeInteger(block);
  if (value > std::numeric_limits<R>::max()) {
    NDN_THROW(tlv::Error("Value in TLV element of type " + to_string(block.type()) + " is too large"));
  }
  return static_cast<R>(value);
}

/** @brief Read a non-negative integer from a TLV element and cast to the specified type.
 *  @tparam R result type, must be an enumeration type
 *  @param block the TLV element
 *  @throw tlv::Error block does not contain a valid non-negative integer or the number cannot be
 *                    represented in R
 *  @warning If R is an unscoped enum type, it must have a fixed underlying type. Otherwise, this
 *           function may trigger unspecified behavior.
 */
template<typename R>
std::enable_if_t<std::is_enum<R>::value, R>
readNonNegativeIntegerAs(const Block& block)
{
  return static_cast<R>(readNonNegativeIntegerAs<std::underlying_type_t<R>>(block));
}

/** @brief Prepend an empty TLV element.
 *  @param encoder an EncodingBuffer or EncodingEstimator
 *  @param type TLV-TYPE number
 *  @details The TLV element has zero-length TLV-VALUE.
 *  @sa makeEmptyBlock
 */
template<Tag TAG>
size_t
prependEmptyBlock(EncodingImpl<TAG>& encoder, uint32_t type);

extern template size_t
prependEmptyBlock<EstimatorTag>(EncodingImpl<EstimatorTag>&, uint32_t);

extern template size_t
prependEmptyBlock<EncoderTag>(EncodingImpl<EncoderTag>&, uint32_t);

/** @brief Create an empty TLV block.
 *  @param type TLV-TYPE number
 *  @return A TLV block with zero-length TLV-VALUE
 *  @sa prependEmptyBlock
 */
Block
makeEmptyBlock(uint32_t type);

/** @brief Prepend a TLV element containing a string.
 *  @param encoder an EncodingBuffer or EncodingEstimator
 *  @param type TLV-TYPE number
 *  @param value string value, may contain NUL octets
 *  @sa makeStringBlock, readString
 */
template<Tag TAG>
size_t
prependStringBlock(EncodingImpl<TAG>& encoder, uint32_t type, const std::string& value);

extern template size_t
prependStringBlock<EstimatorTag>(EncodingImpl<EstimatorTag>&, uint32_t, const std::string&);

extern template size_t
prependStringBlock<EncoderTag>(EncodingImpl<EncoderTag>&, uint32_t, const std::string&);

/** @brief Create a TLV block containing a string.
 *  @param type TLV-TYPE number
 *  @param value string value, may contain NUL octets
 *  @sa prependStringBlock, readString
 */
Block
makeStringBlock(uint32_t type, const std::string& value);

/** @brief Read TLV-VALUE of a TLV element as a string.
 *  @param block the TLV element
 *  @return a string, may contain NUL octets
 *  @sa prependStringBlock, makeStringBlock
 */
std::string
readString(const Block& block);

/** @brief Prepend a TLV element containing an IEEE 754 double-precision floating-point number.
 *  @param encoder an EncodingBuffer or EncodingEstimator
 *  @param type TLV-TYPE number
 *  @param value a floating point number value
 *  @sa makeDoubleBlock, readDouble
 */
template<Tag TAG>
size_t
prependDoubleBlock(EncodingImpl<TAG>& encoder, uint32_t type, double value);

extern template size_t
prependDoubleBlock<EstimatorTag>(EncodingImpl<EstimatorTag>&, uint32_t, double);

extern template size_t
prependDoubleBlock<EncoderTag>(EncodingImpl<EncoderTag>&, uint32_t, double);

/** @brief Create a TLV element containing an IEEE 754 double-precision floating-point number.
 *  @param type TLV-TYPE number
 *  @param value floating point number value
 *  @sa prependDoubleBlock, readDouble
 */
Block
makeDoubleBlock(uint32_t type, double value);

/** @brief Read TLV-VALUE of a TLV element as an IEEE 754 double-precision floating-point number.
 *  @param block the TLV element
 *  @return a floating point number value
 *  @sa prependDoubleBlock, makeDoubleBlock
 */
double
readDouble(const Block& block);

/**
 * @brief Prepend a TLV element containing a sequence of raw bytes.
 * @param encoder an EncodingBuffer or EncodingEstimator
 * @param type TLV-TYPE number
 * @param value range of bytes to use as TLV-VALUE
 * @sa makeBinaryBlock
 */
template<Tag TAG>
size_t
prependBinaryBlock(EncodingImpl<TAG>& encoder, uint32_t type, span<const uint8_t> value);

extern template size_t
prependBinaryBlock<EstimatorTag>(EncodingImpl<EstimatorTag>&, uint32_t, span<const uint8_t>);

extern template size_t
prependBinaryBlock<EncoderTag>(EncodingImpl<EncoderTag>&, uint32_t, span<const uint8_t>);

/**
 * @brief Create a TLV block copying the TLV-VALUE from a byte range.
 * @param type TLV-TYPE number
 * @param value range of bytes to use as TLV-VALUE
 * @sa prependBinaryBlock
 */
Block
makeBinaryBlock(uint32_t type, span<const uint8_t> value);

/** @brief Create a TLV block copying the TLV-VALUE from a raw buffer.
 *  @param type TLV-TYPE number
 *  @param value raw buffer as TLV-VALUE
 *  @param length length of value buffer
 *  @deprecated
 */
inline Block
makeBinaryBlock(uint32_t type, const char* value, size_t length)
{
  return makeBinaryBlock(type, {reinterpret_cast<const uint8_t*>(value), length});
}

namespace detail {

/**
 * @brief Create a binary block copying from RandomAccessIterator.
 */
template<class Iterator>
class BinaryBlockFast
{
public:
  BOOST_CONCEPT_ASSERT((boost::RandomAccessIterator<Iterator>));

  static Block
  makeBlock(uint32_t type, Iterator first, Iterator last)
  {
    EncodingEstimator estimator;
    size_t valueLength = last - first;
    size_t totalLength = valueLength;
    totalLength += estimator.prependVarNumber(valueLength);
    totalLength += estimator.prependVarNumber(type);

    EncodingBuffer encoder(totalLength, 0);
    encoder.prependRange(first, last);
    encoder.prependVarNumber(valueLength);
    encoder.prependVarNumber(type);

    return encoder.block();
  }
};

/**
 * @brief Create a binary block copying from generic InputIterator.
 */
template<class Iterator>
class BinaryBlockSlow
{
public:
  BOOST_CONCEPT_ASSERT((boost::InputIterator<Iterator>));

  static Block
  makeBlock(uint32_t type, Iterator first, Iterator last)
  {
    // reserve 4 bytes in front (common for 1(type)-3(length) encoding
    // Actual size will be adjusted as necessary by the encoder
    EncodingBuffer encoder(4, 4);
    size_t valueLength = encoder.appendRange(first, last);
    encoder.prependVarNumber(valueLength);
    encoder.prependVarNumber(type);

    return encoder.block();
  }
};

} // namespace detail

/** @brief Create a TLV block copying TLV-VALUE from iterators.
 *  @tparam Iterator an InputIterator dereferenceable to a 1-octet type; a faster implementation is
 *                   automatically selected for RandomAccessIterator
 *  @param type TLV-TYPE number
 *  @param first begin iterator
 *  @param last past-the-end iterator
 *  @sa prependBinaryBlock
 */
template<class Iterator>
Block
makeBinaryBlock(uint32_t type, Iterator first, Iterator last)
{
  using BinaryBlockHelper = std::conditional_t<
    std::is_base_of<std::random_access_iterator_tag,
                    typename std::iterator_traits<Iterator>::iterator_category>::value,
    detail::BinaryBlockFast<Iterator>,
    detail::BinaryBlockSlow<Iterator>>;

  return BinaryBlockHelper::makeBlock(type, first, last);
}

/**
 * @brief Prepend a TLV element.
 * @param encoder an EncodingBuffer or EncodingEstimator
 * @param block the TLV element
 */
template<Tag TAG>
size_t
prependBlock(EncodingImpl<TAG>& encoder, const Block& block);

extern template size_t
prependBlock<EstimatorTag>(EncodingImpl<EstimatorTag>&, const Block&);

extern template size_t
prependBlock<EncoderTag>(EncodingImpl<EncoderTag>&, const Block&);

/** @brief Prepend a TLV element containing a nested TLV element.
 *  @tparam U type that satisfies WireEncodableWithEncodingBuffer concept
 *  @param encoder an EncodingBuffer or EncodingEstimator
 *  @param type TLV-TYPE number for outer TLV element
 *  @param value an object to be encoded as inner TLV element
 *  @sa makeNestedBlock
 */
template<Tag TAG, class U>
size_t
prependNestedBlock(EncodingImpl<TAG>& encoder, uint32_t type, const U& value)
{
  BOOST_CONCEPT_ASSERT((WireEncodableWithEncodingBuffer<U>));

  size_t length = value.wireEncode(encoder);
  length += encoder.prependVarNumber(length);
  length += encoder.prependVarNumber(type);
  return length;
}

/** @brief Create a TLV block containing a nested TLV element.
 *  @tparam U type that satisfies WireEncodableWithEncodingBuffer concept
 *  @param type TLV-TYPE number for outer TLV element
 *  @param value an object to be encoded as inner TLV element
 *  @sa prependNestedBlock
 */
template<class U>
Block
makeNestedBlock(uint32_t type, const U& value)
{
  EncodingEstimator estimator;
  size_t totalLength = prependNestedBlock(estimator, type, value);

  EncodingBuffer encoder(totalLength, 0);
  prependNestedBlock(encoder, type, value);
  return encoder.block();
}

/** @brief Prepend a TLV element containing a sequence of nested TLV elements.
 *  @tparam I bidirectional iterator to a type that satisfies WireEncodableWithEncodingBuffer concept
 *  @param encoder an EncodingBuffer or EncodingEstimator
 *  @param type TLV-TYPE number for outer TLV element
 *  @param first begin iterator to inner TLV elements
 *  @param last past-end iterator to inner TLV elements
 *  @sa makeNestedBlock
 */
template<Tag TAG, class I>
size_t
prependNestedBlock(EncodingImpl<TAG>& encoder, uint32_t type, I first, I last)
{
  BOOST_CONCEPT_ASSERT((WireEncodableWithEncodingBuffer<typename std::iterator_traits<I>::value_type>));

  auto rfirst = std::make_reverse_iterator(last);
  auto rlast = std::make_reverse_iterator(first);

  size_t length = 0;
  for (auto i = rfirst; i != rlast; ++i) {
    length += i->wireEncode(encoder);
  }

  length += encoder.prependVarNumber(length);
  length += encoder.prependVarNumber(type);
  return length;
}

/** @brief Create a TLV block containing a sequence of nested TLV elements.
 *  @tparam I bidirectional iterator to a type that satisfies WireEncodableWithEncodingBuffer concept
 *  @param type TLV-TYPE number for outer TLV element
 *  @param first begin iterator to inner TLV elements
 *  @param last past-end iterator to inner TLV elements
 *  @sa prependNestedBlock
 */
template<class I>
Block
makeNestedBlock(uint32_t type, I first, I last)
{
  EncodingEstimator estimator;
  size_t totalLength = prependNestedBlock(estimator, type, first, last);

  EncodingBuffer encoder(totalLength, 0);
  prependNestedBlock(encoder, type, first, last);
  return encoder.block();
}

} // namespace encoding

using encoding::makeNonNegativeIntegerBlock;
using encoding::readNonNegativeInteger;
using encoding::readNonNegativeIntegerAs;
using encoding::makeEmptyBlock;
using encoding::makeStringBlock;
using encoding::readString;
using encoding::makeBinaryBlock;
using encoding::makeNestedBlock;

} // namespace ndn

#endif // NDN_CXX_ENCODING_BLOCK_HELPERS_HPP
