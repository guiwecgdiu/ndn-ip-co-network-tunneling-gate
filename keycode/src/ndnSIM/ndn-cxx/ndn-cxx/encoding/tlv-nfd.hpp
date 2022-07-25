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

#ifndef NDN_CXX_ENCODING_TLV_NFD_HPP
#define NDN_CXX_ENCODING_TLV_NFD_HPP

#include "ndn-cxx/encoding/tlv.hpp"
#include "ndn-cxx/encoding/nfd-constants.hpp"

namespace ndn {
namespace tlv {
namespace nfd {

// NFD Management protocol
enum {
  // ControlParameters
  ControlParameters   = 104,
  FaceId              = 105,
  Uri                 = 114,
  Origin              = 111,
  Cost                = 106,
  Capacity            = 131,
  Count               = 132,
  Flags               = 108,
  Mask                = 112,
  Strategy            = 107,
  ExpirationPeriod    = 109,

  // ControlResponse
  ControlResponse = 101,
  StatusCode      = 102,
  StatusText      = 103,

  // ForwarderStatus
  NfdVersion           = 128,
  StartTimestamp       = 129,
  CurrentTimestamp     = 130,
  NNameTreeEntries     = 131,
  NFibEntries          = 132,
  NPitEntries          = 133,
  NMeasurementsEntries = 134,
  NCsEntries           = 135,

  // Face Management
  FaceStatus                    = 128,
  LocalUri                      = 129,
  ChannelStatus                 = 130,
  UriScheme                     = 131,
  FaceScope                     = 132,
  FacePersistency               = 133,
  LinkType                      = 134,
  BaseCongestionMarkingInterval = 135,
  DefaultCongestionThreshold    = 136,
  Mtu                           = 137,
  FaceQueryFilter               = 150,
  FaceEventNotification         = 192,
  FaceEventKind                 = 193,

  // ForwarderStatus and FaceStatus counters
  NInInterests          = 144,
  NInData               = 145,
  NInNacks              = 151,
  NOutInterests         = 146,
  NOutData              = 147,
  NOutNacks             = 152,
  NInBytes              = 148,
  NOutBytes             = 149,
  NSatisfiedInterests   = 153,
  NUnsatisfiedInterests = 154,

  // Content Store Management
  CsInfo  = 128,
  NHits   = 129,
  NMisses = 130,

  // FIB Management
  FibEntry      = 128,
  NextHopRecord = 129,

  // Strategy Choice Management
  StrategyChoice = 128,

  // RIB Management
  RibEntry = 128,
  Route    = 129
};

} // namespace nfd
} // namespace tlv
} // namespace ndn

#endif // NDN_CXX_ENCODING_TLV_NFD_HPP
