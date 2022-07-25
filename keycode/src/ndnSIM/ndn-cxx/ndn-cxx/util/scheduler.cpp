/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2020 Regents of the University of California.
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

#include "ndn-cxx/util/scheduler.hpp"

#include <boost/scope_exit.hpp>

namespace ndn {
namespace scheduler {

/** \brief Stores internal information about a scheduled event
 */
class EventInfo : noncopyable
{
public:
  EventInfo(time::nanoseconds after, EventCallback&& cb, uint32_t context)
    : callback(std::move(cb))
    , expireTime(time::steady_clock::now() + after)
    , context(context)
  {
  }

  NDN_CXX_NODISCARD time::nanoseconds
  expiresFromNow() const
  {
    return std::max(expireTime - time::steady_clock::now(), 0_ns);
  }

public:
  EventCallback callback;
  Scheduler::EventQueue::const_iterator queueIt;
  time::steady_clock::TimePoint expireTime;
  bool isExpired = false;
  uint32_t context = 0;
};

EventId::EventId(Scheduler& sched, weak_ptr<EventInfo> info)
  : CancelHandle([&sched, info] { sched.cancelImpl(info.lock()); })
  , m_info(std::move(info))
{
}

EventId::operator bool() const noexcept
{
  auto sp = m_info.lock();
  return sp != nullptr && !sp->isExpired;
}

void
EventId::reset() noexcept
{
  *this = {};
}

std::ostream&
operator<<(std::ostream& os, const EventId& eventId)
{
  return os << eventId.m_info.lock();
}

bool
Scheduler::EventQueueCompare::operator()(const shared_ptr<EventInfo>& a,
                                         const shared_ptr<EventInfo>& b) const noexcept
{
  return a->expireTime < b->expireTime;
}

Scheduler::Scheduler(DummyIoService& ioService)
{
}

Scheduler::~Scheduler()
{
  cancelAllEvents();
}

EventId
Scheduler::schedule(time::nanoseconds after, EventCallback callback)
{
  BOOST_ASSERT(callback != nullptr);

  auto i = m_queue.insert(std::make_shared<EventInfo>(after, std::move(callback), ns3::Simulator::GetContext()));
  (*i)->queueIt = i;

  if (!m_isEventExecuting && i == m_queue.begin()) {
    // the new event is the first one to expire
    scheduleNext();
  }

  return EventId(*this, *i);
}

void
Scheduler::cancelImpl(const shared_ptr<EventInfo>& info)
{
  if (info == nullptr || info->isExpired) {
    return;
  }

  if (m_timerEvent) {
    if (!m_timerEvent->IsExpired()) {
      ns3::Simulator::Remove(*m_timerEvent);
    }
    m_timerEvent.reset();
  }
  m_queue.erase(info->queueIt);

  if (!m_isEventExecuting) {
    scheduleNext();
  }
}

void
Scheduler::cancelAllEvents()
{
  m_queue.clear();
  if (m_timerEvent) {
    if (!m_timerEvent->IsExpired()) {
      ns3::Simulator::Remove(*m_timerEvent);
    }
    m_timerEvent.reset();
  }
}

void
Scheduler::scheduleNext()
{
  if (!m_queue.empty()) {
    m_timerEvent = ns3::Simulator::Schedule(ns3::NanoSeconds((*m_queue.begin())->expiresFromNow().count()),
                                            &Scheduler::executeEvent, this);
  }
}

void
Scheduler::executeEvent()
{
  m_isEventExecuting = true;

  m_timerEvent.reset();
  BOOST_SCOPE_EXIT(this_) {
    this_->m_isEventExecuting = false;
    this_->scheduleNext();
  } BOOST_SCOPE_EXIT_END

  // process all expired events
  auto now = time::steady_clock::now();
  while (!m_queue.empty()) {
    auto head = m_queue.begin();
    shared_ptr<EventInfo> info = *head;
    if (info->expireTime > now) {
      break;
    }

    m_queue.erase(head);
    info->isExpired = true;
    if (ns3::Simulator::GetContext() == info->context) {
      info->callback();
    }
    else {
      ns3::Simulator::ScheduleWithContext(info->context, ns3::Seconds(0), ns3::MakeEvent(info->callback));
    }
  }
}

} // namespace scheduler
} // namespace ndn
