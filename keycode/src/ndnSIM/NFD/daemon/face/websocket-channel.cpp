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

#include "websocket-channel.hpp"
#include "face.hpp"
#include "generic-link-service.hpp"
#include "websocket-transport.hpp"
#include "common/global.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT(WebSocketChannel);

WebSocketChannel::WebSocketChannel(const websocket::Endpoint& localEndpoint)
  : m_localEndpoint(localEndpoint)
  , m_pingInterval(10_s)
{
  setUri(FaceUri(m_localEndpoint, "ws"));
  NFD_LOG_CHAN_INFO("Creating channel");

  // Be quiet
  m_server.clear_access_channels(websocketpp::log::alevel::all);
  m_server.clear_error_channels(websocketpp::log::elevel::all);

  // Setup WebSocket server
  m_server.init_asio(&getGlobalIoService());
  m_server.set_tcp_pre_bind_handler([isV6 = m_localEndpoint.address().is_v6()] (const auto& acceptor) {
    if (isV6) {
      acceptor->set_option(boost::asio::ip::v6_only(true));
    }
    return websocketpp::lib::error_code{};
  });
  m_server.set_open_handler(std::bind(&WebSocketChannel::handleOpen, this, _1));
  m_server.set_close_handler(std::bind(&WebSocketChannel::handleClose, this, _1));
  m_server.set_message_handler(std::bind(&WebSocketChannel::handleMessage, this, _1, _2));

  // Detect disconnections using ping-pong messages
  m_server.set_pong_handler(std::bind(&WebSocketChannel::handlePong, this, _1));
  m_server.set_pong_timeout_handler(std::bind(&WebSocketChannel::handlePongTimeout, this, _1));

  // Always set SO_REUSEADDR flag
  m_server.set_reuse_addr(true);
}

void
WebSocketChannel::setPingInterval(time::milliseconds interval)
{
  BOOST_ASSERT(!m_server.is_listening());

  m_pingInterval = interval;
}

void
WebSocketChannel::setPongTimeout(time::milliseconds timeout)
{
  BOOST_ASSERT(!m_server.is_listening());

  m_server.set_pong_timeout(static_cast<long>(timeout.count()));
}

void
WebSocketChannel::handlePongTimeout(websocketpp::connection_hdl hdl)
{
  auto it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end()) {
    static_cast<WebSocketTransport*>(it->second->getTransport())->handlePongTimeout();
  }
  else {
    NFD_LOG_CHAN_WARN("Pong timeout on unknown transport");
  }
}

void
WebSocketChannel::handlePong(websocketpp::connection_hdl hdl)
{
  auto it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end()) {
    static_cast<WebSocketTransport*>(it->second->getTransport())->handlePong();
  }
  else {
    NFD_LOG_CHAN_WARN("Pong received on unknown transport");
  }
}

void
WebSocketChannel::handleMessage(websocketpp::connection_hdl hdl,
                                websocket::Server::message_ptr msg)
{
  auto it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end()) {
    static_cast<WebSocketTransport*>(it->second->getTransport())->receiveMessage(msg->get_payload());
  }
  else {
    NFD_LOG_CHAN_WARN("Message received on unknown transport");
  }
}

void
WebSocketChannel::handleOpen(websocketpp::connection_hdl hdl)
{
  NFD_LOG_CHAN_TRACE("Incoming connection from " << m_server.get_con_from_hdl(hdl)->get_remote_endpoint());

  auto linkService = make_unique<GenericLinkService>();
  auto transport = make_unique<WebSocketTransport>(hdl, m_server, m_pingInterval);
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));
  face->setChannel(shared_from_this()); // use weak_from_this() in C++17

  BOOST_ASSERT(m_channelFaces.count(hdl) == 0);
  m_channelFaces[hdl] = face;
  connectFaceClosedSignal(*face, [this, hdl] { m_channelFaces.erase(hdl); });

  m_onFaceCreatedCallback(face);
}

void
WebSocketChannel::handleClose(websocketpp::connection_hdl hdl)
{
  auto it = m_channelFaces.find(hdl);
  if (it != m_channelFaces.end()) {
    it->second->close();
  }
  else {
    NFD_LOG_CHAN_WARN("Close on unknown transport");
  }
}

void
WebSocketChannel::listen(const FaceCreatedCallback& onFaceCreated)
{
  if (isListening()) {
    NFD_LOG_CHAN_WARN("Already listening");
    return;
  }

  m_onFaceCreatedCallback = onFaceCreated;

  m_server.listen(m_localEndpoint);
  m_server.start_accept();
  NFD_LOG_CHAN_DEBUG("Started listening");
}

} // namespace face
} // namespace nfd
