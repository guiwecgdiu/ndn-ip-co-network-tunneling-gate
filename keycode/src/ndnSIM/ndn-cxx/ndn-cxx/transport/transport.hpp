/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2022 Regents of the University of California.
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

#ifndef NDN_CXX_TRANSPORT_TRANSPORT_HPP
#define NDN_CXX_TRANSPORT_TRANSPORT_HPP

#include "ndn-cxx/detail/asio-fwd.hpp"
#include "ndn-cxx/detail/common.hpp"
#include "ndn-cxx/encoding/block.hpp"

#include <boost/system/error_code.hpp>

namespace ndn {

/** \brief Provides TLV-block delivery service.
 */
class Transport : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;

    Error(const boost::system::error_code& code, const std::string& msg);
  };

  using ReceiveCallback = std::function<void(const Block& wire)>;
  using ErrorCallback = std::function<void()>;

  virtual
  ~Transport() = default;

  /** \brief Asynchronously open the connection.
   *  \param receiveCallback callback function when a TLV block is received; must not be empty
   *  \throw boost::system::system_error connection cannot be established
   */
  virtual void
  connect(ReceiveCallback receiveCallback);

  /**
   * \brief Close the connection.
   */
  virtual void
  close() = 0;

  /**
   * \brief Send a TLV block through the transport.
   */
  virtual void
  send(const Block& block) = 0;

  /**
   * \brief Pause the transport, canceling all pending operations.
   * \post the receive callback will not be invoked
   * \note This operation has no effect if the transport has been paused,
   *       or when the connection is being established.
   */
  virtual void
  pause() = 0;

  /**
   * \brief Resume the transport.
   * \post the receive callback will be invoked
   * \note This operation has no effect if the transport is not paused,
   *       or when the connection is being established.
   */
  virtual void
  resume() = 0;

  /**
   * \brief Return whether the transport is connected.
   */
  bool
  isConnected() const noexcept
  {
    return m_isConnected;
  }

  /**
   * \retval true incoming packets are expected, the receive callback will be invoked
   * \retval false incoming packets are not expected, the receive callback will not be invoked
   */
  bool
  isReceiving() const noexcept
  {
    return m_isReceiving;
  }

protected:
  ReceiveCallback m_receiveCallback;
  bool m_isConnected = false;
  bool m_isReceiving = false;
};

} // namespace ndn

#endif // NDN_CXX_TRANSPORT_TRANSPORT_HPP
