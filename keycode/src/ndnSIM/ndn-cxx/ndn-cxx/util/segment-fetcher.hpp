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

#ifndef NDN_CXX_UTIL_SEGMENT_FETCHER_HPP
#define NDN_CXX_UTIL_SEGMENT_FETCHER_HPP

#include "ndn-cxx/face.hpp"
#include "ndn-cxx/security/validator.hpp"
#include "ndn-cxx/util/rtt-estimator.hpp"
#include "ndn-cxx/util/scheduler.hpp"
#include "ndn-cxx/util/signal.hpp"

#include <queue>
#include <set>

namespace ndn {
namespace util {

/**
 * @brief Utility class to fetch the latest version of a segmented object.
 *
 * SegmentFetcher assumes that segments in the object are named `/<prefix>/<version>/<segment>`,
 * where:
 * - `<prefix>` is the specified prefix,
 * - `<version>` is an unknown version that needs to be discovered, and
 * - `<segment>` is a segment number (the number of segments in the object is unknown until a Data
 *   packet containing the `FinalBlockId` field is received).
 *
 * SegmentFetcher implements the following logic:
 *
 * 1. Express an Interest to discover the latest version of the object:
 *
 *    Interest: `/<prefix>?CanBePrefix&MustBeFresh`
 *
 * 2. Infer the latest version of the object: `<version> = Data.getName().get(-2)`.
 *
 * 3. Keep sending Interests for future segments until an error occurs or the number of segments
 *    indicated by the FinalBlockId in a received Data packet is reached. This retrieval will start
 *    at segment 1 if segment 0 was received in response to the Interest expressed in step 2;
 *    otherwise, retrieval will start at segment 0. By default, congestion control will be used to
 *    manage the Interest window size. Interests expressed in this step will follow this Name
 *    format: `/<prefix>/<version>/<segment=(N)>`.
 *
 * 4. If set to 'block' mode, signal #onComplete passing a memory buffer that combines the content
 *    of all segments in the object. If set to 'in order' mode, signal #onInOrderData is triggered
 *    upon validation of each segment in segment order, storing later segments that arrived out of
 *    order internally until all earlier segments have arrived and have been validated.
 *
 * If an error occurs during the fetching process, #onError is signaled with one of the error codes
 * from SegmentFetcher::ErrorCode.
 *
 * A Validator instance must be specified to validate individual segments. Every time a segment has
 * been successfully validated, #afterSegmentValidated will be signaled.
 *
 * Example:
 * @code
 *   auto fetcher = SegmentFetcher::start(face, Interest("/data/prefix"), validator);
 *   fetcher->onComplete.connect([] (ConstBufferPtr data) {...});
 *   fetcher->onError.connect([] (uint32_t errorCode, const std::string& errorMsg) {...});
 * @endcode
 */
class SegmentFetcher : noncopyable
{
public:
  /**
   * @brief Error codes passed to #onError.
   */
  enum ErrorCode {
    /// Retrieval timed out because the maximum timeout between the successful receipt of segments was exceeded
    INTEREST_TIMEOUT = 1,
    /// One of the retrieved Data packets lacked a segment number in the last Name component (excl. implicit digest)
    DATA_HAS_NO_SEGMENT = 2,
    /// One of the retrieved segments failed user-provided validation
    SEGMENT_VALIDATION_FAIL = 3,
    /// An unrecoverable Nack was received during retrieval
    NACK_ERROR = 4,
    /// A received FinalBlockId did not contain a segment component
    FINALBLOCKID_NOT_SEGMENT = 5,
  };

  class Options
  {
  public:
    Options()
    {
    }

    void
    validate();

  public:
    time::milliseconds interestLifetime = 4_s; ///< lifetime of sent Interests - independent of Interest timeout
    time::milliseconds maxTimeout = 60_s; ///< maximum allowed time between successful receipt of segments
    bool inOrder = false; ///< true for 'in order' mode, false for 'block' mode
    bool useConstantInterestTimeout = false; ///< if true, Interest timeout is kept at `maxTimeout`
    bool useConstantCwnd = false; ///< if true, window size is kept at `initCwnd`
    bool disableCwa = false; ///< disable Conservative Window Adaptation
    bool resetCwndToInit = false; ///< reduce cwnd to initCwnd when loss event occurs
    bool ignoreCongMarks = false; ///< disable window decrease after congestion mark received
    double initCwnd = 1.0; ///< initial congestion window size
    double initSsthresh = std::numeric_limits<double>::max(); ///< initial slow start threshold
    double aiStep = 1.0; ///< additive increase step (in segments)
    double mdCoef = 0.5; ///< multiplicative decrease coefficient
    RttEstimator::Options rttOptions; ///< options for RTT estimator
    size_t flowControlWindow = 25000; ///< maximum number of segments stored in the reorder buffer
  };

  /**
   * @brief Initiates segment fetching.
   *
   * Transfer completion, failure, and progress are indicated via signals.
   *
   * @param face         Reference to the Face that should be used to fetch data.
   * @param baseInterest Interest for the initial segment of requested data.
   *                     This interest may include a custom InterestLifetime and parameters that
   *                     will propagate to all subsequent Interests. The only exception is that the
   *                     initial Interest will be forced to include the "CanBePrefix=true" and
   *                     "MustBeFresh=true" parameters, which will not be included in subsequent
   *                     Interests.
   * @param validator    Reference to the Validator the fetcher will use to validate data.
   *                     The caller must ensure the validator remains valid until either #onComplete
   *                     or #onError has been signaled.
   * @param options      Options controlling the transfer.
   *
   * @return             A shared_ptr to the constructed SegmentFetcher.
   *                     This shared_ptr is kept internally for the lifetime of the transfer.
   *                     Therefore, it does not need to be saved and is provided here so that the
   *                     SegmentFetcher's signals can be connected to.
   */
  static shared_ptr<SegmentFetcher>
  start(Face& face,
        const Interest& baseInterest,
        security::Validator& validator,
        const Options& options = Options());

  /**
   * @brief Stops fetching.
   *
   * This cancels all interests that are still pending.
   */
  void
  stop();

private:
  class PendingSegment;

  SegmentFetcher(Face& face, security::Validator& validator, const Options& options);

  static bool
  shouldStop(const weak_ptr<SegmentFetcher>& weakSelf);

  void
  fetchFirstSegment(const Interest& baseInterest, bool isRetransmission);

  void
  fetchSegmentsInWindow(const Interest& origInterest);

  void
  sendInterest(uint64_t segNum, const Interest& interest, bool isRetransmission);

  void
  afterSegmentReceivedCb(const Interest& origInterest, const Data& data,
                         const weak_ptr<SegmentFetcher>& weakSelf);

  void
  afterValidationSuccess(const Data& data, const Interest& origInterest,
                         std::map<uint64_t, PendingSegment>::iterator pendingSegmentIt,
                         const weak_ptr<SegmentFetcher>& weakSelf);

  void
  afterValidationFailure(const Data& data,
                         const security::ValidationError& error,
                         const weak_ptr<SegmentFetcher>& weakSelf);

  void
  afterNackReceivedCb(const Interest& origInterest, const lp::Nack& nack,
                      const weak_ptr<SegmentFetcher>& weakSelf);

  void
  afterTimeoutCb(const Interest& origInterest,
                 const weak_ptr<SegmentFetcher>& weakSelf);

  void
  afterNackOrTimeout(const Interest& origInterest);

  void
  finalizeFetch();

  void
  windowIncrease();

  void
  windowDecrease();

  void
  signalError(uint32_t code, const std::string& msg);

  void
  updateRetransmittedSegment(uint64_t segmentNum,
                             const PendingInterestHandle& pendingInterest,
                             scheduler::EventId timeoutEvent);

  void
  cancelExcessInFlightSegments();

  bool
  checkAllSegmentsReceived();

  time::milliseconds
  getEstimatedRto();

public:
  /**
   * @brief Emitted upon successful retrieval of the complete object (all segments).
   * @note Emitted only if SegmentFetcher is operating in 'block' mode.
   */
  Signal<SegmentFetcher, ConstBufferPtr> onComplete;

  /**
   * @brief Emitted when the retrieval could not be completed due to an error.
   *
   * Handlers are provided with an error code and a string error message.
   */
  Signal<SegmentFetcher, uint32_t, std::string> onError;

  /**
   * @brief Emitted whenever a data segment received.
   */
  Signal<SegmentFetcher, Data> afterSegmentReceived;

  /**
   * @brief Emitted whenever a received data segment has been successfully validated.
   */
  Signal<SegmentFetcher, Data> afterSegmentValidated;

  /**
   * @brief Emitted whenever an Interest for a data segment is nacked.
   */
  Signal<SegmentFetcher> afterSegmentNacked;

  /**
   * @brief Emitted whenever an Interest for a data segment times out.
   */
  Signal<SegmentFetcher> afterSegmentTimedOut;

  /**
   * @brief Emitted after each data segment in segment order has been validated.
   * @note Emitted only if SegmentFetcher is operating in 'in order' mode.
   */
  Signal<SegmentFetcher, ConstBufferPtr> onInOrderData;

  /**
   * @brief Emitted on successful retrieval of all segments in 'in order' mode.
   * @note Emitted only if SegmentFetcher is operating in 'in order' mode.
   */
  Signal<SegmentFetcher> onInOrderComplete;

private:
  enum class SegmentState {
    FirstInterest, ///< the first Interest for this segment has been sent
    InRetxQueue,   ///< the segment is awaiting Interest retransmission
    Retransmitted, ///< one or more retransmitted Interests have been sent for this segment
  };

  class PendingSegment
  {
  public:
    SegmentState state;
    time::steady_clock::TimePoint sendTime;
    ScopedPendingInterestHandle hdl;
    scheduler::ScopedEventId timeoutEvent;
  };

NDN_CXX_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  static constexpr double MIN_SSTHRESH = 2.0;

  shared_ptr<SegmentFetcher> m_this;

  Options m_options;
  Face& m_face;
  Scheduler m_scheduler;
  security::Validator& m_validator;
  RttEstimator m_rttEstimator;

  time::steady_clock::TimePoint m_timeLastSegmentReceived;
  std::queue<uint64_t> m_retxQueue;
  Name m_versionedDataName;
  uint64_t m_nextSegmentNum = 0;
  double m_cwnd;
  double m_ssthresh;
  int64_t m_nSegmentsInFlight = 0;
  int64_t m_nSegments = 0;
  uint64_t m_highInterest = 0;
  uint64_t m_highData = 0;
  uint64_t m_recPoint = 0;
  int64_t m_nReceived = 0;
  int64_t m_nBytesReceived = 0;
  uint64_t m_nextSegmentInOrder = 0;

  std::map<uint64_t, Buffer> m_segmentBuffer;
  std::map<uint64_t, PendingSegment> m_pendingSegments;
  std::set<uint64_t> m_receivedSegments;
};

} // namespace util
} // namespace ndn

#endif // NDN_CXX_UTIL_SEGMENT_FETCHER_HPP
