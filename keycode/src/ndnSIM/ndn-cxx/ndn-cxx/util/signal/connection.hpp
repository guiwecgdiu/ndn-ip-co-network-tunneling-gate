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

#ifndef NDN_CXX_UTIL_SIGNAL_CONNECTION_HPP
#define NDN_CXX_UTIL_SIGNAL_CONNECTION_HPP

#include "ndn-cxx/detail/common.hpp"

namespace ndn {
namespace util {
namespace signal {

using DisconnectFunction = std::function<void()>;

/** \brief represents a connection to a signal
 *  \note This type is copyable. Any copy can be used to disconnect.
 */
class Connection
{
public:
  constexpr
  Connection() noexcept = default;

  /** \brief disconnects from the signal
   *  \note If the connection is already disconnected, or if the Signal has been destructed,
   *        this operation has no effect.
   *  \warning During signal emission, attempting to disconnect a connection other than
   *           the executing handler's own connection results in undefined behavior.
   */
  void
  disconnect();

  /** \brief check if connected to the signal
   */
  bool
  isConnected() const noexcept
  {
    return !m_disconnect.expired();
  }

private:
  /** \param disconnect weak_ptr to a function that disconnects the handler
   */
  explicit
  Connection(weak_ptr<DisconnectFunction> disconnect) noexcept;

  template<typename Owner, typename ...TArgs>
  friend class Signal;

private:
  // NOTE: the following "hidden friend" operators are available via
  //       argument-dependent lookup only and must be defined inline.

  /** \brief compare for equality
   *
   *  Two connections are equal if they both refer to the same connection that isn't disconnected,
   *  or they are both disconnected.
   */
  friend bool
  operator==(const Connection& lhs, const Connection& rhs) noexcept
  {
    return (!lhs.isConnected() && !rhs.isConnected()) ||
        (!lhs.m_disconnect.owner_before(rhs.m_disconnect) &&
         !rhs.m_disconnect.owner_before(lhs.m_disconnect));
  }

  friend bool
  operator!=(const Connection& lhs, const Connection& rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  /** \note The only shared_ptr to the disconnect function is stored in Signal<..>::Slot,
   *        and will be destructed if the handler is disconnected (through another Connection
   *        instance) or the Signal is destructed.
   *        Connection needs a weak_ptr instead of a shared_ptr to the disconnect function,
   *        because the disconnect function is bound with an iterator to the Slot,
   *        which is invalidated when the Slot is erased.
   */
  weak_ptr<DisconnectFunction> m_disconnect;
};

} // namespace signal
} // namespace util
} // namespace ndn

#endif // NDN_CXX_UTIL_SIGNAL_CONNECTION_HPP
