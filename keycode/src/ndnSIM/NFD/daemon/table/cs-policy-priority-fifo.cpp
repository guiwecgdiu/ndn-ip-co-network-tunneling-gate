/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "cs-policy-priority-fifo.hpp"
#include "cs.hpp"
#include "common/global.hpp"

namespace nfd {
namespace cs {
namespace priority_fifo {

const std::string PriorityFifoPolicy::POLICY_NAME = "priority_fifo";
NFD_REGISTER_CS_POLICY(PriorityFifoPolicy);

PriorityFifoPolicy::PriorityFifoPolicy()
  : Policy(POLICY_NAME)
{
}

PriorityFifoPolicy::~PriorityFifoPolicy()
{
  for (auto entryInfoMapPair : m_entryInfoMap) {
    delete entryInfoMapPair.second;
  }
}

void
PriorityFifoPolicy::doAfterInsert(EntryRef i)
{
  this->attachQueue(i);
  this->evictEntries();
}

void
PriorityFifoPolicy::doAfterRefresh(EntryRef i)
{
  this->detachQueue(i);
  this->attachQueue(i);
}

void
PriorityFifoPolicy::doBeforeErase(EntryRef i)
{
  this->detachQueue(i);
}

void
PriorityFifoPolicy::doBeforeUse(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());
}

void
PriorityFifoPolicy::evictEntries()
{
  BOOST_ASSERT(this->getCs() != nullptr);

  while (this->getCs()->size() > this->getLimit()) {
    this->evictOne();
  }
}

void
PriorityFifoPolicy::evictOne()
{
  BOOST_ASSERT(!m_queues[QUEUE_UNSOLICITED].empty() ||
               !m_queues[QUEUE_STALE].empty() ||
               !m_queues[QUEUE_FIFO].empty());

  EntryRef i;
  if (!m_queues[QUEUE_UNSOLICITED].empty()) {
    i = m_queues[QUEUE_UNSOLICITED].front();
  }
  else if (!m_queues[QUEUE_STALE].empty()) {
    i = m_queues[QUEUE_STALE].front();
  }
  else if (!m_queues[QUEUE_FIFO].empty()) {
    i = m_queues[QUEUE_FIFO].front();
  }

  this->detachQueue(i);
  this->emitSignal(beforeEvict, i);
}

void
PriorityFifoPolicy::attachQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) == m_entryInfoMap.end());

  EntryInfo* entryInfo = new EntryInfo();
  if (i->isUnsolicited()) {
    entryInfo->queueType = QUEUE_UNSOLICITED;
  }
  else if (!i->isFresh()) {
    entryInfo->queueType = QUEUE_STALE;
  }
  else {
    entryInfo->queueType = QUEUE_FIFO;
    entryInfo->moveStaleEventId = getScheduler().schedule(i->getData().getFreshnessPeriod(),
                                                          [=] { moveToStaleQueue(i); });
  }

  Queue& queue = m_queues[entryInfo->queueType];
  entryInfo->queueIt = queue.insert(queue.end(), i);
  m_entryInfoMap[i] = entryInfo;
}

void
PriorityFifoPolicy::detachQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());

  EntryInfo* entryInfo = m_entryInfoMap[i];
  if (entryInfo->queueType == QUEUE_FIFO) {
    entryInfo->moveStaleEventId.cancel();
  }

  m_queues[entryInfo->queueType].erase(entryInfo->queueIt);
  m_entryInfoMap.erase(i);
  delete entryInfo;
}

void
PriorityFifoPolicy::moveToStaleQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());

  EntryInfo* entryInfo = m_entryInfoMap[i];
  BOOST_ASSERT(entryInfo->queueType == QUEUE_FIFO);

  m_queues[QUEUE_FIFO].erase(entryInfo->queueIt);

  entryInfo->queueType = QUEUE_STALE;
  Queue& queue = m_queues[QUEUE_STALE];
  entryInfo->queueIt = queue.insert(queue.end(), i);
  m_entryInfoMap[i] = entryInfo;
}

} // namespace priority_fifo
} // namespace cs
} // namespace nfd
