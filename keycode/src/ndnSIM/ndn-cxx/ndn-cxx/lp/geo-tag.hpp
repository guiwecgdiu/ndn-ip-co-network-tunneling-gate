/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef NDN_CXX_LP_GEO_TAG_HPP
#define NDN_CXX_LP_GEO_TAG_HPP

#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ndn-cxx/tag.hpp"

namespace ndn {
namespace lp {

/**
 * \brief represents a GeoTag header field
 */
class GeoTag : public Tag
{
public:
  static constexpr int
  getTypeId() noexcept
  {
    return 0x60000001;
  }

  GeoTag() = default;

  explicit
  GeoTag(std::tuple<double, double, double> pos)
    : m_pos(pos)
  {
  }

  explicit
  GeoTag(const Block& block);

  /**
   * \brief prepend GeoTag to encoder
   */
  template<encoding::Tag TAG>
  size_t
  wireEncode(EncodingImpl<TAG>& encoder) const;

  /**
   * \brief encode GeoTag into wire format
   */
  const Block&
  wireEncode() const;

  /**
   * \brief get GeoTag from wire format
   */
  void
  wireDecode(const Block& wire);

public: // get & set GeoTag
  /**
   * \return position x of GeoTag
   */
  std::tuple<double, double, double>
  getPos() const
  {
    return m_pos;
  }

  /**
   * \brief set position x
   */
  GeoTag*
  setPosX(std::tuple<double, double, double> pos)
  {
    m_pos = pos;
    return this;
  }

private:
  std::tuple<double, double, double> m_pos = {0.0, 0.0, 0.0};
  mutable Block m_wire;
};

} // namespace lp
} // namespace ndn

#endif // NDN_CXX_LP_GEOTAG_HPP
