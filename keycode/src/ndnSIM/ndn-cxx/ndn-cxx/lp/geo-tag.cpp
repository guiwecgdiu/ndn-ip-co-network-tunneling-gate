/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2019 Regents of the University of California.
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

#include "ndn-cxx/lp/geo-tag.hpp"
#include "ndn-cxx/lp/tlv.hpp"

namespace ndn {
namespace lp {

GeoTag::GeoTag(const Block& block)
{
  wireDecode(block);
}

template<encoding::Tag TAG>
size_t
GeoTag::wireEncode(EncodingImpl<TAG>& encoder) const
{
  size_t length = 0;
  length += prependDoubleBlock(encoder, tlv::GeoTagPos, std::get<2>(m_pos));
  length += prependDoubleBlock(encoder, tlv::GeoTagPos, std::get<1>(m_pos));
  length += prependDoubleBlock(encoder, tlv::GeoTagPos, std::get<0>(m_pos));
  length += encoder.prependVarNumber(length);
  length += encoder.prependVarNumber(tlv::GeoTag);
  return length;
}

template size_t
GeoTag::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>& encoder) const;

template size_t
GeoTag::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>& encoder) const;

const Block&
GeoTag::wireEncode() const
{
  if (m_wire.hasWire()) {
    return m_wire;
  }

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();

  return m_wire;
}

void
GeoTag::wireDecode(const Block& wire)
{
  if (wire.type() != tlv::GeoTag) {
    NDN_THROW(ndn::tlv::Error("expecting GeoTag block"));
  }

  m_wire = wire;
  m_wire.parse();

  if (m_wire.elements().size() < 3 ||
      m_wire.elements()[0].type() != tlv::GeoTagPos ||
      m_wire.elements()[1].type() != tlv::GeoTagPos ||
      m_wire.elements()[2].type() != tlv::GeoTagPos) {
    NDN_THROW(ndn::tlv::Error("Unexpected input while decoding GeoTag"));
  }
  m_pos = {encoding::readDouble(m_wire.elements()[0]),
           encoding::readDouble(m_wire.elements()[1]),
           encoding::readDouble(m_wire.elements()[2])};
}

} // namespace lp
} // namespace ndn
