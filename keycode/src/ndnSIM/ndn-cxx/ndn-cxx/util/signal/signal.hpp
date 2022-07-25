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

#ifndef NDN_CXX_UTIL_SIGNAL_SIGNAL_HPP
#define NDN_CXX_UTIL_SIGNAL_SIGNAL_HPP

#include "ndn-cxx/util/scope.hpp"
#include "ndn-cxx/util/signal/connection.hpp"

#include <list>

namespace ndn {
namespace util {
namespace signal {

class DummyExtraArg;

/** \brief provides a lightweight signal / event system
 *
 *  To declare a signal:
 *    public:
 *      Signal<Owner, T1, T2> signalName;
 *  To connect to a signal:
 *    owner->signalName.connect(f);
 *    Multiple functions can connect to the same signal.
 *  To emit a signal from owner:
 *    this->signalName(arg1, arg2);
 *
 *  \tparam Owner the signal owner class; only this class can emit the signal
 *  \tparam TArgs types of signal arguments
 *  \sa signal-emit.hpp allows owner's derived classes to emit signals
 */
template<typename Owner, typename ...TArgs>
class Signal : noncopyable
{
public: // API for anyone
  /** \brief represents a function that can connect to the signal
   */
  typedef function<void(const TArgs&...)> Handler;

  Signal();

  ~Signal();

  /** \brief connects a handler to the signal
   *  \note If invoked from a handler, the new handler won't receive the current emitted signal.
   *  \warning The handler is permitted to disconnect itself, but it must ensure its validity.
   */
  Connection
  connect(Handler handler);

  /** \brief connects a single-shot handler to the signal
   *
   *  After the handler is executed once, it is automatically disconnected.
   */
  Connection
  connectSingleShot(Handler handler);

private: // API for owner
  /** \retval true if there is no connection
   */
  bool
  isEmpty() const;

  /** \brief emits a signal
   *  \param args arguments passed to all handlers
   *  \warning Emitting the signal from a handler is undefined behavior.
   *  \warning Destructing the Signal object during signal emission is undefined behavior.
   *  \note If a handler throws, the exception will be propagated to the caller
   *        who emits this signal, and some handlers may not be executed.
   */
  void
  operator()(const TArgs&... args);

  /** \brief (implementation detail) emits a signal
   *  \note This overload is used by signal-emit.hpp.
   */
  void
  operator()(const TArgs&... args, const DummyExtraArg&);

  // make Owner a friend of Signal<Owner, ...> so that API for owner can be called
  friend Owner;

private: // internal implementation
  typedef Signal<Owner, TArgs...> Self;

  /** \brief stores a handler function, and a function to disconnect this handler
   */
  struct Slot
  {
    /** \brief the handler function who will receive emitted signals
     */
    Handler handler;

    /** \brief the disconnect function which will disconnect this handler
     *
     *  In practice this is the Signal::disconnect method bound to an iterator
     *  pointing at this slot.
     *
     *  This is the only shared_ptr to this function object.
     *  Connection class has a weak_ptr which references the same function object.
     *  When the slot is erased or the signal is destructed, this function object is
     *  destructed, and the related Connections cannot disconnect this slot again.
     */
    shared_ptr<DisconnectFunction> disconnect;
  };

  /** \brief stores slots
   *  \note std::list is used because iterators must not be invalidated
   *        when other slots are added or removed
   */
  typedef std::list<Slot> SlotList;
  SlotList m_slots;

  /** \brief is a signal handler executing?
   */
  bool m_isExecuting;

  /** \brief iterator to current executing slot
   *  \note This field is meaningful when isExecuting==true
   */
  typename SlotList::iterator m_currentSlot;

  /** \brief disconnects the handler in a slot
   */
  void
  disconnect(typename SlotList::iterator it);
};

template<typename Owner, typename ...TArgs>
Signal<Owner, TArgs...>::Signal()
  : m_isExecuting(false)
{
}

template<typename Owner, typename ...TArgs>
Signal<Owner, TArgs...>::~Signal()
{
  BOOST_ASSERT(!m_isExecuting);
}

template<typename Owner, typename ...TArgs>
Connection
Signal<Owner, TArgs...>::connect(Handler handler)
{
  auto it = m_slots.insert(m_slots.end(), {std::move(handler), nullptr});
  it->disconnect = make_shared<DisconnectFunction>([=] { disconnect(it); });

  return signal::Connection(it->disconnect);
}

template<typename Owner, typename ...TArgs>
Connection
Signal<Owner, TArgs...>::connectSingleShot(Handler handler)
{
  auto it = m_slots.insert(m_slots.end(), {nullptr, nullptr});
  it->disconnect = make_shared<DisconnectFunction>([=] { disconnect(it); });
  signal::Connection conn(it->disconnect);

  it->handler = [conn, handler = std::move(handler)] (const TArgs&... args) mutable {
    handler(args...);
    conn.disconnect();
  };

  return conn;
}

template<typename Owner, typename ...TArgs>
void
Signal<Owner, TArgs...>::disconnect(typename SlotList::iterator it)
{
  if (m_isExecuting) {
    // during signal emission, only the currently executing handler can be disconnected
    BOOST_ASSERT_MSG(it == m_currentSlot, "cannot disconnect another handler from a handler");

    // this serves to indicate that the current slot needs to be erased from the list
    // after it finishes executing; we cannot do it here because of bug #2333
    m_currentSlot = m_slots.end();

    // expire all weak_ptrs, to prevent double disconnections
    it->disconnect.reset();
  }
  else {
    m_slots.erase(it);
  }
}

template<typename Owner, typename ...TArgs>
bool
Signal<Owner, TArgs...>::isEmpty() const
{
  return !m_isExecuting && m_slots.empty();
}

template<typename Owner, typename ...TArgs>
void
Signal<Owner, TArgs...>::operator()(const TArgs&... args)
{
  BOOST_ASSERT_MSG(!m_isExecuting, "cannot emit signal from a handler");

  if (m_slots.empty()) {
    return;
  }

  auto guard = make_scope_exit([this] { m_isExecuting = false; });
  m_isExecuting = true;

  auto it = m_slots.begin();
  auto last = std::prev(m_slots.end());
  bool isLast = false;
  while (!isLast) {
    m_currentSlot = it;
    isLast = it == last;

    m_currentSlot->handler(args...);

    if (m_currentSlot == m_slots.end())
      it = m_slots.erase(it);
    else
      ++it;
  }
}

template<typename Owner, typename ...TArgs>
void
Signal<Owner, TArgs...>::operator()(const TArgs&... args, const DummyExtraArg&)
{
  this->operator()(args...);
}

} // namespace signal

// expose as ndn::util::Signal
using signal::Signal;

} // namespace util
} // namespace ndn

#endif // NDN_CXX_UTIL_SIGNAL_SIGNAL_HPP
