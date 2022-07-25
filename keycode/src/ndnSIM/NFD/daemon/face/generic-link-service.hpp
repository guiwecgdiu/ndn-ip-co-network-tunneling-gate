/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP
#define NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP

#include "link-service.hpp"
#include "lp-fragmenter.hpp"
#include "lp-reassembler.hpp"
#include "lp-reliability.hpp"

#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace face {

/** \brief counters provided by GenericLinkService
 *  \note The type name 'GenericLinkServiceCounters' is implementation detail.
 *        Use 'GenericLinkService::Counters' in public API.
 */
class GenericLinkServiceCounters : public virtual LinkService::Counters
{
public:
  /** \brief count of failed fragmentations
   */
  PacketCounter nFragmentationErrors;

  /** \brief count of outgoing LpPackets dropped due to exceeding MTU limit
   *
   *  If this counter is non-zero, the operator should enable fragmentation.
   */
  PacketCounter nOutOverMtu;

  /** \brief count of invalid LpPackets dropped before reassembly
   */
  PacketCounter nInLpInvalid;

  /** \brief count of network-layer packets currently being reassembled
   */
  SizeCounter<LpReassembler> nReassembling;

  /** \brief count of dropped partial network-layer packets due to reassembly timeout
   */
  PacketCounter nReassemblyTimeouts;

  /** \brief count of invalid reassembled network-layer packets dropped
   */
  PacketCounter nInNetInvalid;

  /** \brief count of network-layer packets that did not require retransmission of a fragment
   */
  PacketCounter nAcknowledged;

  /** \brief count of network-layer packets that had at least one fragment retransmitted, but were
   *         eventually received in full
   */
  PacketCounter nRetransmitted;

  /** \brief count of network-layer packets dropped because a fragment reached the maximum number
   *         of retransmissions
   */
  PacketCounter nRetxExhausted;

  /** \brief count of LpPackets dropped due to duplicate Sequence numbers
   */
  PacketCounter nDuplicateSequence;

  /** \brief count of outgoing LpPackets that were marked with congestion marks
   */
  PacketCounter nCongestionMarked;
};

/** \brief GenericLinkService is a LinkService that implements the NDNLPv2 protocol
 *  \sa https://redmine.named-data.net/projects/nfd/wiki/NDNLPv2
 */
class GenericLinkService NFD_FINAL_UNLESS_WITH_TESTS : public LinkService
                                                     , protected virtual GenericLinkServiceCounters
{
public:
  /** \brief Options that control the behavior of GenericLinkService
   */
  class Options
  {
  public:
    Options() noexcept
    {
    }

  public:
    /** \brief enables encoding of IncomingFaceId, and decoding of NextHopFaceId and CachePolicy
     */
    bool allowLocalFields = false;

    /** \brief enables fragmentation
     */
    bool allowFragmentation = false;

    /** \brief options for fragmentation
     */
    LpFragmenter::Options fragmenterOptions;

    /** \brief enables reassembly
     */
    bool allowReassembly = false;

    /** \brief options for reassembly
     */
    LpReassembler::Options reassemblerOptions;

    /** \brief options for reliability
     */
    LpReliability::Options reliabilityOptions;

    /** \brief enables send queue congestion detection and marking
     */
    bool allowCongestionMarking = false;

    /** \brief starting value for congestion marking interval
     *
     *  Packets are marked if the queue size stays above THRESHOLD for at least one INTERVAL.
     *
     *  The default value (100 ms) is taken from RFC 8289 (CoDel).
     */
    time::nanoseconds baseCongestionMarkingInterval = 100_ms;

    /** \brief default congestion threshold in bytes
     *
     *  Packets are marked if the queue size stays above THRESHOLD for at least one INTERVAL.
     *
     *  The default value (64 KiB) works well for a queue capacity of 200 KiB.
     */
    size_t defaultCongestionThreshold = 65536;

    /** \brief enables self-learning forwarding support
     */
    bool allowSelfLearning = true;

    /** \brief overrides MTU provided by Transport
     *
     *  This MTU value will be used instead of the MTU provided by the transport if it is less than
     *  the transport MTU. However, it will not be utilized when the transport MTU is unlimited.
     *
     *  Acceptable values for the override MTU are values >= MIN_MTU, which can be validated before
     *  being set with canOverrideMtuTo().
     */
    ssize_t overrideMtu = std::numeric_limits<ssize_t>::max();

    /** \brief Enable encoding and decoding of GeoTags
     *
     *  To enable, set value of enableGeoTags option to a function that generates `shared_ptr<GeoTag>`
     */
    std::function<std::shared_ptr<ndn::lp::GeoTag>()> enableGeoTags;
  };

  /** \brief counters provided by GenericLinkService
   */
  using Counters = GenericLinkServiceCounters;

  explicit
  GenericLinkService(const Options& options = {});

  /** \brief get Options used by GenericLinkService
   */
  const Options&
  getOptions() const;

  /** \brief sets Options used by GenericLinkService
   */
  void
  setOptions(const Options& options);

  const Counters&
  getCounters() const NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL;

  ssize_t
  getEffectiveMtu() const NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL;

  /** \brief Whether MTU can be overridden to the specified value
   *
   *  If the transport MTU is unlimited, then this will always return false.
   */
  bool
  canOverrideMtuTo(ssize_t mtu) const;

NFD_PROTECTED_WITH_TESTS_ELSE_PRIVATE: // send path
  /** \brief request an IDLE packet to transmit pending service fields
   */
  void
  requestIdlePacket();

  /** \brief send an LpPacket
   */
  void
  sendLpPacket(lp::Packet&& pkt);

  void
  doSendInterest(const Interest& interest) NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL;

  void
  doSendData(const Data& data) NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL;

  void
  doSendNack(const ndn::lp::Nack& nack) NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL;

  /** \brief assign consecutive sequence numbers to LpPackets
   */
  void
  assignSequences(std::vector<lp::Packet>& pkts);

private: // send path
  /** \brief encode link protocol fields from tags onto an outgoing LpPacket
   *  \param netPkt network-layer packet to extract tags from
   *  \param lpPacket LpPacket to add link protocol fields to
   */
  void
  encodeLpFields(const ndn::PacketBase& netPkt, lp::Packet& lpPacket);

  /** \brief send a complete network layer packet
   *  \param pkt LpPacket containing a complete network layer packet
   *  \param isInterest whether the network layer packet is an Interest
   */
  void
  sendNetPacket(lp::Packet&& pkt, bool isInterest);

  /** \brief if the send queue is found to be congested, add a congestion mark to the packet
   *         according to CoDel
   *  \sa https://tools.ietf.org/html/rfc8289
   */
  void
  checkCongestionLevel(lp::Packet& pkt);

private: // receive path
  void
  doReceivePacket(const Block& packet, const EndpointId& endpoint) NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL;

  /** \brief decode incoming network-layer packet
   *  \param netPkt reassembled network-layer packet
   *  \param firstPkt LpPacket of first fragment
   *  \param endpointId endpoint of peer who sent the packet
   *
   *  If decoding is successful, a receive signal is emitted;
   *  otherwise, a warning is logged.
   */
  void
  decodeNetPacket(const Block& netPkt, const lp::Packet& firstPkt, const EndpointId& endpointId);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Interest
   *  \param firstPkt LpPacket of first fragment; must not have Nack field
   *  \param endpointId endpoint of peer who sent the Interest
   *
   *  If decoding is successful, receiveInterest signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeInterest(const Block& netPkt, const lp::Packet& firstPkt, const EndpointId& endpointId);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Data
   *  \param firstPkt LpPacket of first fragment
   *  \param endpointId endpoint of peer who sent the Data
   *
   *  If decoding is successful, receiveData signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeData(const Block& netPkt, const lp::Packet& firstPkt, const EndpointId& endpointId);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Interest
   *  \param firstPkt LpPacket of first fragment; must have Nack field
   *  \param endpointId endpoint of peer who sent the Nack
   *
   *  If decoding is successful, receiveNack signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeNack(const Block& netPkt, const lp::Packet& firstPkt, const EndpointId& endpointId);

NFD_PROTECTED_WITH_TESTS_ELSE_PRIVATE:
  Options m_options;
  LpFragmenter m_fragmenter;
  LpReassembler m_reassembler;
  LpReliability m_reliability;
  lp::Sequence m_lastSeqNo;

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /// Time to mark next packet due to send queue congestion
  time::steady_clock::TimePoint m_nextMarkTime;
  /// number of marked packets in the current incident of congestion
  size_t m_nMarkedSinceInMarkingState;

  friend class LpReliability;
};

inline const GenericLinkService::Options&
GenericLinkService::getOptions() const
{
  return m_options;
}

inline const GenericLinkService::Counters&
GenericLinkService::getCounters() const
{
  return *this;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP
