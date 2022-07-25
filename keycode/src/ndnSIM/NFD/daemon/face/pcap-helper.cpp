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

#include "pcap-helper.hpp"
#include "ethernet-protocol.hpp"

#include "common/privilege-helper.hpp"

#include <pcap/pcap.h>
#include <unistd.h>

#if !defined(PCAP_NETMASK_UNKNOWN)
#define PCAP_NETMASK_UNKNOWN  0xffffffff
#endif

namespace nfd {
namespace face {

PcapHelper::PcapHelper(const std::string& interfaceName)
  : m_pcap(nullptr)
{
  char errbuf[PCAP_ERRBUF_SIZE] = {};
  m_pcap = pcap_create(interfaceName.data(), errbuf);
  if (!m_pcap)
    NDN_THROW(Error("pcap_create: " + std::string(errbuf)));

  // Enable "immediate mode", effectively disabling any read buffering in the kernel.
  // This corresponds to the BIOCIMMEDIATE ioctl on BSD-like systems (including macOS)
  // where libpcap uses a BPF device. On Linux this forces libpcap not to use TPACKET_V3,
  // even if the kernel supports it, thus preventing bug #1511.
  if (pcap_set_immediate_mode(m_pcap, 1) < 0)
    NDN_THROW(Error("pcap_set_immediate_mode failed"));

  pcap_set_snaplen(m_pcap, ethernet::HDR_LEN + ndn::MAX_NDN_PACKET_SIZE);
  pcap_set_buffer_size(m_pcap, 4 * 1024 * 1024);
}

PcapHelper::~PcapHelper()
{
  close();
}

void
PcapHelper::activate(int dlt)
{
  PrivilegeHelper::runElevated([this] {
    int ret = pcap_activate(m_pcap);
    if (ret < 0)
      NDN_THROW(Error("pcap_activate: " + std::string(pcap_statustostr(ret))));
  });

  if (pcap_set_datalink(m_pcap, dlt) < 0)
    NDN_THROW(Error("pcap_set_datalink: " + getLastError()));

  if (pcap_setdirection(m_pcap, PCAP_D_IN) < 0)
    NDN_THROW(Error("pcap_setdirection: " + getLastError()));
}

void
PcapHelper::close() noexcept
{
  if (m_pcap) {
    pcap_close(m_pcap);
    m_pcap = nullptr;
  }
}

int
PcapHelper::getFd() const
{
  int fd = pcap_get_selectable_fd(m_pcap);
  if (fd < 0)
    NDN_THROW(Error("pcap_get_selectable_fd failed"));

  // we need to duplicate the fd, otherwise both pcap_close() and the
  // caller may attempt to close the same fd and one of them will fail
  return ::dup(fd);
}

std::string
PcapHelper::getLastError() const noexcept
{
  return pcap_geterr(m_pcap);
}

size_t
PcapHelper::getNDropped() const
{
  pcap_stat ps{};
  if (pcap_stats(m_pcap, &ps) < 0)
    NDN_THROW(Error("pcap_stats: " + getLastError()));

  return ps.ps_drop;
}

void
PcapHelper::setPacketFilter(const char* filter) const
{
  bpf_program prog;
  if (pcap_compile(m_pcap, &prog, filter, 1, PCAP_NETMASK_UNKNOWN) < 0)
    NDN_THROW(Error("pcap_compile: " + getLastError()));

  int ret = pcap_setfilter(m_pcap, &prog);
  pcap_freecode(&prog);
  if (ret < 0)
    NDN_THROW(Error("pcap_setfilter: " + getLastError()));
}

std::tuple<span<const uint8_t>, std::string>
PcapHelper::readNextPacket() const noexcept
{
  pcap_pkthdr* header;
  const uint8_t* packet;

  int ret = pcap_next_ex(m_pcap, &header, &packet);
  if (ret < 0)
    return {span<uint8_t>{}, getLastError()};
  else if (ret == 0)
    return {span<uint8_t>{}, "timed out"};
  else
    return {{packet, header->caplen}, ""};
}

} // namespace face
} // namespace nfd
