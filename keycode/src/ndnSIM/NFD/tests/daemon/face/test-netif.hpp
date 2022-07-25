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

#ifndef NFD_TESTS_DAEMON_FACE_TEST_NETIF_HPP
#define NFD_TESTS_DAEMON_FACE_TEST_NETIF_HPP

#include "core/common.hpp"

#include <ndn-cxx/net/network-address.hpp>
#include <ndn-cxx/net/network-interface.hpp>
#include <ndn-cxx/net/network-monitor.hpp>

namespace nfd {
namespace face {
namespace tests {

using ndn::net::NetworkAddress;
using ndn::net::NetworkInterface;
using ndn::net::NetworkMonitor;

/** \brief Enumerate network interfaces using the given NetworkMonitor
 *  \param netmon a NetworkMonitor constructed on the global io_service.
 *  \note This function is blocking
 *  \note Signals are supported if caller keeps \p netmon running
 */
std::vector<shared_ptr<const NetworkInterface>>
enumerateNetworkInterfaces(NetworkMonitor& netmon);

/** \brief Collect information about network interfaces
 *  \param allowCached if true, previously collected information can be returned
 *  \note This function is blocking if \p allowCached is false or no previous information exists
 *  \warning Signals are not triggered on returned NetworkInterfaces because NetworkMonitor is not running
 */
std::vector<shared_ptr<const NetworkInterface>>
collectNetworkInterfaces(bool allowCached = true);

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_TEST_NETIF_HPP
