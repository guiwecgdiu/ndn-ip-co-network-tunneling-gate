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

#ifndef NFD_DAEMON_FACE_WEBSOCKET_FACTORY_HPP
#define NFD_DAEMON_FACE_WEBSOCKET_FACTORY_HPP

#include "protocol-factory.hpp"
#include "websocket-channel.hpp"

namespace nfd {
namespace face {

/** \brief Protocol factory for WebSocket
 */
class WebSocketFactory final : public ProtocolFactory
{
public:
  static const std::string&
  getId() noexcept;

  using ProtocolFactory::ProtocolFactory;

  /**
   * \brief Create WebSocket-based channel using websocket::Endpoint
   *
   * websocket::Endpoint is really an alias for boost::asio::ip::tcp::endpoint.
   *
   * If this method called twice with the same endpoint, only one channel
   * will be created.  The second call will just retrieve the existing
   * channel.
   *
   * \returns always a valid pointer to a WebSocketChannel object, an exception
   *          is thrown if it cannot be created.
   */
  shared_ptr<WebSocketChannel>
  createChannel(const websocket::Endpoint& localEndpoint);

private:
  /** \brief process face_system.websocket config section
   */
  void
  doProcessConfig(OptionalConfigSection configSection,
                  FaceSystem::ConfigContext& context) final;

  std::vector<shared_ptr<const Channel>>
  doGetChannels() const final;

private:
  std::map<websocket::Endpoint, shared_ptr<WebSocketChannel>> m_channels;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_WEBSOCKET_FACTORY_HPP
