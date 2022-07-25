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

#include "algorithm.hpp"
#include "scope-prefix.hpp"

namespace nfd {
namespace fw {

bool
wouldViolateScope(const Face& inFace, const Interest& interest, const Face& outFace)
{
  if (outFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
    // forwarding to a local face is always allowed
    return false;
  }

  if (scope_prefix::LOCALHOST.isPrefixOf(interest.getName())) {
    // localhost Interests cannot be forwarded to a non-local face
    return true;
  }

  if (scope_prefix::LOCALHOP.isPrefixOf(interest.getName())) {
    // localhop Interests can be forwarded to a non-local face only if it comes from a local face
    return inFace.getScope() != ndn::nfd::FACE_SCOPE_LOCAL;
  }

  // Interest name is not subject to scope control
  return false;
}

int
findDuplicateNonce(const pit::Entry& pitEntry, Interest::Nonce nonce, const Face& face)
{
  int dnw = DUPLICATE_NONCE_NONE;

  for (const pit::InRecord& inRecord : pitEntry.getInRecords()) {
    if (inRecord.getLastNonce() == nonce) {
      if (&inRecord.getFace() == &face) {
        dnw |= DUPLICATE_NONCE_IN_SAME;
      }
      else {
        dnw |= DUPLICATE_NONCE_IN_OTHER;
      }
    }
  }

  for (const pit::OutRecord& outRecord : pitEntry.getOutRecords()) {
    if (outRecord.getLastNonce() == nonce) {
      if (&outRecord.getFace() == &face) {
        dnw |= DUPLICATE_NONCE_OUT_SAME;
      }
      else {
        dnw |= DUPLICATE_NONCE_OUT_OTHER;
      }
    }
  }

  return dnw;
}

bool
hasPendingOutRecords(const pit::Entry& pitEntry)
{
  auto now = time::steady_clock::now();
  return std::any_of(pitEntry.out_begin(), pitEntry.out_end(),
                      [&now] (const pit::OutRecord& outRecord) {
                        return outRecord.getExpiry() >= now &&
                               outRecord.getIncomingNack() == nullptr;
                      });
}

time::steady_clock::time_point
getLastOutgoing(const pit::Entry& pitEntry)
{
  pit::OutRecordCollection::const_iterator lastOutgoing = std::max_element(
    pitEntry.out_begin(), pitEntry.out_end(),
    [] (const pit::OutRecord& a, const pit::OutRecord& b) {
      return a.getLastRenewed() < b.getLastRenewed();
    });
  BOOST_ASSERT(lastOutgoing != pitEntry.out_end());

  return lastOutgoing->getLastRenewed();
}

fib::NextHopList::const_iterator
findEligibleNextHopWithEarliestOutRecord(const Face& inFace, const Interest& interest,
                                         const fib::NextHopList& nexthops,
                                         const shared_ptr<pit::Entry>& pitEntry)
{
  auto found = nexthops.end();
  auto earliestRenewed = time::steady_clock::time_point::max();

  for (auto it = nexthops.begin(); it != nexthops.end(); ++it) {
    if (!isNextHopEligible(inFace, interest, *it, pitEntry))
      continue;

    auto outRecord = pitEntry->getOutRecord(it->getFace());
    BOOST_ASSERT(outRecord != pitEntry->out_end());
    if (outRecord->getLastRenewed() < earliestRenewed) {
      found = it;
      earliestRenewed = outRecord->getLastRenewed();
    }
  }
  return found;
}

bool
isNextHopEligible(const Face& inFace, const Interest& interest,
                  const fib::NextHop& nexthop,
                  const shared_ptr<pit::Entry>& pitEntry,
                  bool wantUnused,
                  time::steady_clock::time_point now)
{
  const Face& outFace = nexthop.getFace();

  // do not forward back to the same face, unless it is ad hoc
  if ((outFace.getId() == inFace.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
     (wouldViolateScope(inFace, interest, outFace)))
    return false;

  if (wantUnused) {
    // nexthop must not have unexpired out-record
    auto outRecord = pitEntry->getOutRecord(outFace);
    if (outRecord != pitEntry->out_end() && outRecord->getExpiry() > now) {
      return false;
    }
  }
  return true;
}

} // namespace fw
} // namespace nfd
