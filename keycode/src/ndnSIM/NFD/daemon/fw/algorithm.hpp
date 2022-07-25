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

#ifndef NFD_DAEMON_FW_ALGORITHM_HPP
#define NFD_DAEMON_FW_ALGORITHM_HPP

#include "table/fib-entry.hpp"
#include "table/pit-entry.hpp"

/** \file
 *  This file contains common algorithms used by forwarding strategies.
 */

namespace nfd {
namespace fw {

/** \brief determine whether forwarding the Interest in \p pitEntry to \p outFace would violate scope
 *  \sa https://redmine.named-data.net/projects/nfd/wiki/ScopeControl
 */
bool
wouldViolateScope(const Face& inFace, const Interest& interest, const Face& outFace);

/** \brief indicates where duplicate Nonces are found
 */
enum DuplicateNonceWhere {
  DUPLICATE_NONCE_NONE      = 0,        ///< no duplicate Nonce is found
  DUPLICATE_NONCE_IN_SAME   = (1 << 0), ///< in-record of same face
  DUPLICATE_NONCE_IN_OTHER  = (1 << 1), ///< in-record of other face
  DUPLICATE_NONCE_OUT_SAME  = (1 << 2), ///< out-record of same face
  DUPLICATE_NONCE_OUT_OTHER = (1 << 3), ///< out-record of other face
};

/** \brief determine whether \p pitEntry has duplicate Nonce \p nonce
 *  \return OR'ed DuplicateNonceWhere
 */
int
findDuplicateNonce(const pit::Entry& pitEntry, Interest::Nonce nonce, const Face& face);

/** \brief determine whether \p pitEntry has any pending out-records
 *  \return true if there is at least one out-record waiting for Data
 */
bool
hasPendingOutRecords(const pit::Entry& pitEntry);

/** \return last out-record time
 *  \pre pitEntry has one or more unexpired out-records
 */
time::steady_clock::TimePoint
getLastOutgoing(const pit::Entry& pitEntry);

/** \brief pick an eligible NextHop with earliest out-record
 *  \note It is assumed that every nexthop has an out-record.
 */
fib::NextHopList::const_iterator
findEligibleNextHopWithEarliestOutRecord(const Face& inFace, const Interest& interest,
                                         const fib::NextHopList& nexthops,
                                         const shared_ptr<pit::Entry>& pitEntry);

/** \brief determines whether a NextHop is eligible i.e. not the same inFace
 *  \param inFace incoming face of current Interest
 *  \param interest incoming Interest
 *  \param nexthop next hop
 *  \param pitEntry PIT entry
 *  \param wantUnused if true, NextHop must not have unexpired out-record
 *  \param now time::steady_clock::now(), ignored if !wantUnused
 */
bool
isNextHopEligible(const Face& inFace, const Interest& interest,
                  const fib::NextHop& nexthop,
                  const shared_ptr<pit::Entry>& pitEntry,
                  bool wantUnused = false,
                  time::steady_clock::TimePoint now = time::steady_clock::TimePoint::min());

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_ALGORITHM_HPP
