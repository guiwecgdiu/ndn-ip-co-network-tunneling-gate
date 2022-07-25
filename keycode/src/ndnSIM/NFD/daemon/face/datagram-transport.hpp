/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FACE_DATAGRAM_TRANSPORT_HPP
#define NFD_DAEMON_FACE_DATAGRAM_TRANSPORT_HPP

#include "transport.hpp"
#include "socket-utils.hpp"
#include "common/global.hpp"

#include <array>

namespace nfd {
namespace face {

struct Unicast {};
struct Multicast {};

/** \brief Implements Transport for datagram-based protocols.
 *
 *  \tparam Protocol a datagram-based protocol in Boost.Asio
 */
template<class Protocol, class Addressing = Unicast>
class DatagramTransport : public Transport
{
public:
  using protocol = Protocol;

  /** \brief Construct datagram transport.
   *
   *  \param socket Protocol-specific socket for the created transport
   */
  explicit
  DatagramTransport(typename protocol::socket&& socket);

  ssize_t
  getSendQueueLength() override;

  /**
   * \brief Receive datagram, translate buffer into packet, deliver to parent class.
   */
  void
  receiveDatagram(span<const uint8_t> buffer, const boost::system::error_code& error);

protected:
  void
  doClose() override;

  void
  doSend(const Block& packet) override;

  void
  handleSend(const boost::system::error_code& error, size_t nBytesSent);

  void
  handleReceive(const boost::system::error_code& error, size_t nBytesReceived);

  void
  processErrorCode(const boost::system::error_code& error);

  bool
  hasRecentlyReceived() const;

  void
  resetRecentlyReceived();

  static EndpointId
  makeEndpointId(const typename protocol::endpoint& ep);

protected:
  typename protocol::socket m_socket;
  typename protocol::endpoint m_sender;

  NFD_LOG_MEMBER_DECL();

private:
  std::array<uint8_t, ndn::MAX_NDN_PACKET_SIZE> m_receiveBuffer;
  bool m_hasRecentlyReceived;
};


template<class T, class U>
DatagramTransport<T, U>::DatagramTransport(typename DatagramTransport::protocol::socket&& socket)
  : m_socket(std::move(socket))
  , m_hasRecentlyReceived(false)
{
  boost::asio::socket_base::send_buffer_size sendBufferSizeOption;
  boost::system::error_code error;
  m_socket.get_option(sendBufferSizeOption, error);
  if (error) {
    NFD_LOG_FACE_WARN("Failed to obtain send queue capacity from socket: " << error.message());
    this->setSendQueueCapacity(QUEUE_ERROR);
  }
  else {
    this->setSendQueueCapacity(sendBufferSizeOption.value());
  }

  m_socket.async_receive_from(boost::asio::buffer(m_receiveBuffer), m_sender,
                              [this] (auto&&... args) {
                                this->handleReceive(std::forward<decltype(args)>(args)...);
                              });
}

template<class T, class U>
ssize_t
DatagramTransport<T, U>::getSendQueueLength()
{
  ssize_t queueLength = getTxQueueLength(m_socket.native_handle());
  if (queueLength == QUEUE_ERROR) {
    NFD_LOG_FACE_WARN("Failed to obtain send queue length from socket: " << std::strerror(errno));
  }
  return queueLength;
}

template<class T, class U>
void
DatagramTransport<T, U>::doClose()
{
  NFD_LOG_FACE_TRACE(__func__);

  if (m_socket.is_open()) {
    // Cancel all outstanding operations and close the socket.
    // Use the non-throwing variants and ignore errors, if any.
    boost::system::error_code error;
    m_socket.cancel(error);
    m_socket.close(error);
  }

  // Ensure that the Transport stays alive at least until
  // all pending handlers are dispatched
  getGlobalIoService().post([this] {
    this->setState(TransportState::CLOSED);
  });
}

template<class T, class U>
void
DatagramTransport<T, U>::doSend(const Block& packet)
{
  NFD_LOG_FACE_TRACE(__func__);

  m_socket.async_send(boost::asio::buffer(packet),
                      // 'packet' is copied into the lambda to retain the underlying Buffer
                      [this, packet] (auto&&... args) {
                        this->handleSend(std::forward<decltype(args)>(args)...);
                      });
}

template<class T, class U>
void
DatagramTransport<T, U>::receiveDatagram(span<const uint8_t> buffer,
                                         const boost::system::error_code& error)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Received: " << buffer.size() << " bytes from " << m_sender);

  bool isOk = false;
  Block element;
  std::tie(isOk, element) = Block::fromBuffer(buffer);
  if (!isOk) {
    NFD_LOG_FACE_WARN("Failed to parse incoming packet from " << m_sender);
    // This packet won't extend the face lifetime
    return;
  }
  if (element.size() != buffer.size()) {
    NFD_LOG_FACE_WARN("Received datagram size and decoded element size don't match");
    // This packet won't extend the face lifetime
    return;
  }
  m_hasRecentlyReceived = true;

  this->receive(element, makeEndpointId(m_sender));
}

template<class T, class U>
void
DatagramTransport<T, U>::handleReceive(const boost::system::error_code& error, size_t nBytesReceived)
{
  receiveDatagram(ndn::make_span(m_receiveBuffer).first(nBytesReceived), error);

  if (m_socket.is_open())
    m_socket.async_receive_from(boost::asio::buffer(m_receiveBuffer), m_sender,
                                [this] (auto&&... args) {
                                  this->handleReceive(std::forward<decltype(args)>(args)...);
                                });
}

template<class T, class U>
void
DatagramTransport<T, U>::handleSend(const boost::system::error_code& error, size_t nBytesSent)
{
  if (error)
    return processErrorCode(error);

  NFD_LOG_FACE_TRACE("Successfully sent: " << nBytesSent << " bytes");
}

template<class T, class U>
void
DatagramTransport<T, U>::processErrorCode(const boost::system::error_code& error)
{
  NFD_LOG_FACE_TRACE(__func__);

  if (getState() == TransportState::CLOSING ||
      getState() == TransportState::FAILED ||
      getState() == TransportState::CLOSED ||
      error == boost::asio::error::operation_aborted) {
    // transport is shutting down, ignore any errors
    return;
  }

  if (getPersistency() == ndn::nfd::FACE_PERSISTENCY_PERMANENT) {
    NFD_LOG_FACE_DEBUG("Permanent face ignores error: " << error.message());
    return;
  }

  NFD_LOG_FACE_ERROR("Send or receive operation failed: " << error.message());
  this->setState(TransportState::FAILED);
  doClose();
}

template<class T, class U>
bool
DatagramTransport<T, U>::hasRecentlyReceived() const
{
  return m_hasRecentlyReceived;
}

template<class T, class U>
void
DatagramTransport<T, U>::resetRecentlyReceived()
{
  m_hasRecentlyReceived = false;
}

template<class T, class U>
EndpointId
DatagramTransport<T, U>::makeEndpointId(const typename protocol::endpoint&)
{
  return 0;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_DATAGRAM_TRANSPORT_HPP
