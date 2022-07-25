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

#ifndef NFD_DAEMON_MGMT_CS_MANAGER_HPP
#define NFD_DAEMON_MGMT_CS_MANAGER_HPP

#include "manager-base.hpp"

namespace nfd {

namespace cs {
class Cs;
} // namespace cs

class ForwarderCounters;

/**
 * \brief Implements the CS Management of NFD Management Protocol.
 * \sa https://redmine.named-data.net/projects/nfd/wiki/CsMgmt
 */
class CsManager final : public ManagerBase
{
public:
  CsManager(cs::Cs& cs, const ForwarderCounters& fwCounters,
            Dispatcher& dispatcher, CommandAuthenticator& authenticator);

private:
  /** \brief Process cs/config command.
   */
  void
  changeConfig(const ControlParameters& parameters,
               const ndn::mgmt::CommandContinuation& done);

  /** \brief Process cs/erase command.
   */
  void
  erase(const ControlParameters& parameters,
        const ndn::mgmt::CommandContinuation& done);

  /** \brief Serve CS information dataset.
   */
  void
  serveInfo(const Name& topPrefix, const Interest& interest,
            ndn::mgmt::StatusDatasetContext& context) const;

public:
  static constexpr size_t ERASE_LIMIT = 256;

private:
  cs::Cs& m_cs;
  const ForwarderCounters& m_fwCounters;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_CS_MANAGER_HPP
