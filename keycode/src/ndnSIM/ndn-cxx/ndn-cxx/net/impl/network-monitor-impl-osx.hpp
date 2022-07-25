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

#ifndef NDN_CXX_NET_NETWORK_MONITOR_IMPL_OSX_HPP
#define NDN_CXX_NET_NETWORK_MONITOR_IMPL_OSX_HPP

#include "ndn-cxx/detail/config.hpp"
#include "ndn-cxx/net/network-monitor.hpp"

#ifndef NDN_CXX_HAVE_OSX_FRAMEWORKS
#error "This file should not be included ..."
#endif

#include "ndn-cxx/detail/cf-releaser-osx.hpp"
#include "ndn-cxx/util/scheduler.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>

#include <boost/asio/ip/udp.hpp>
#include <map>

namespace ndn {
namespace net {

class IfAddrs;

class NetworkMonitorImplOsx : public NetworkMonitorImpl
{
public:
  NetworkMonitorImplOsx(boost::asio::io_service& io);

  ~NetworkMonitorImplOsx();

  uint32_t
  getCapabilities() const final
  {
    return NetworkMonitor::CAP_ENUM |
           NetworkMonitor::CAP_IF_ADD_REMOVE |
           NetworkMonitor::CAP_STATE_CHANGE |
           NetworkMonitor::CAP_ADDR_ADD_REMOVE;
  }

  shared_ptr<const NetworkInterface>
  getNetworkInterface(const std::string& ifname) const final;

  std::vector<shared_ptr<const NetworkInterface>>
  listNetworkInterfaces() const final;

private:
  static void
  afterNotificationCenterEvent(CFNotificationCenterRef center, void* observer,
                               CFStringRef name, const void* object,
                               CFDictionaryRef userInfo);

  void
  scheduleCfLoop();

  void
  enumerateInterfaces();

  std::set<std::string>
  getInterfaceNames() const;

  void
  addNewInterface(const std::string& ifName, const IfAddrs& ifaList);

  InterfaceState
  getInterfaceState(const NetworkInterface& netif) const;

  size_t
  getInterfaceMtu(const NetworkInterface& netif);

  void
  updateInterfaceInfo(NetworkInterface& netif, const IfAddrs& ifaList);

  static void
  onConfigChanged(SCDynamicStoreRef store, CFArrayRef changedKeys, void* context);

  void
  onConfigChanged(CFArrayRef changedKeys);

private:
  std::map<std::string, shared_ptr<NetworkInterface>> m_interfaces; ///< ifname => interface

  Scheduler m_scheduler;
  scheduler::ScopedEventId m_cfLoopEvent;

  SCDynamicStoreContext m_context;
  detail::CFReleaser<SCDynamicStoreRef> m_scStore;
  detail::CFReleaser<CFRunLoopSourceRef> m_loopSource;

  boost::asio::ip::udp::socket m_ioctlSocket;
};

} // namespace net
} // namespace ndn

#endif // NDN_CXX_NET_NETWORK_MONITOR_IMPL_OSX_HPP
