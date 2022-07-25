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

#include "service.hpp"

#include "fib-updater.hpp"
#include "readvertise/client-to-nlsr-readvertise-policy.hpp"
#include "readvertise/host-to-gateway-readvertise-policy.hpp"
#include "readvertise/nfd-rib-readvertise-destination.hpp"
#include "readvertise/readvertise.hpp"

#include "common/global.hpp"
#include "common/logger.hpp"

#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"

namespace nfd {
namespace rib {

NFD_LOG_INIT(RibService);

const std::string CFG_RIB = "rib";
const std::string CFG_LOCALHOST_SECURITY = "localhost_security";
const std::string CFG_LOCALHOP_SECURITY = "localhop_security";
const std::string CFG_PA_VALIDATION = "prefix_announcement_validation";
const std::string CFG_PREFIX_PROPAGATE = "auto_prefix_propagate";
const std::string CFG_READVERTISE_NLSR = "readvertise_nlsr";
const Name READVERTISE_NLSR_PREFIX = "/localhost/nlsr";
const uint64_t PROPAGATE_DEFAULT_COST = 15;
const time::milliseconds PROPAGATE_DEFAULT_TIMEOUT = 10_s;

Service::Service(const ConfigSection& configSection, ndn::Face& face, ndn::KeyChain& keyChain)
  : Service(keyChain, face,
            [&configSection] (ConfigFile& config, bool isDryRun) {
              config.parse(configSection, isDryRun, "internal://nfd.conf");
            })
{
}

template<typename ConfigParseFunc>
Service::Service(ndn::KeyChain& keyChain, ndn::Face& face, const ConfigParseFunc& configParse)
  : m_keyChain(keyChain)
  , m_face(face)
  , m_scheduler(m_face.getIoService())
  , m_nfdController(m_face, m_keyChain)
  , m_fibUpdater(m_rib, m_nfdController)
  , m_dispatcher(m_face, m_keyChain)
  , m_ribManager(m_rib, m_face, m_keyChain, m_nfdController, m_dispatcher)
{
  ConfigFile config(ConfigFile::ignoreUnknownSection);
  config.addSectionHandler(CFG_RIB, [this] (auto&&... args) {
    processConfig(std::forward<decltype(args)>(args)...);
  });
  configParse(config, true);
  configParse(config, false);

  m_ribManager.registerWithNfd();
  m_ribManager.enableLocalFields();
}

Service::~Service()
{
}

Service&
Service::get()
{
  auto node = ::ns3::NodeList::GetNode(::ns3::Simulator::GetContext());
  auto l3 = node->GetObject<::ns3::ndn::L3Protocol>();
  return l3->getRibService();
}

void
Service::processConfig(const ConfigSection& section, bool isDryRun, const std::string& filename)
{
  if (isDryRun) {
    checkConfig(section, filename);
  }
  else {
    applyConfig(section, filename);
  }
}

void
Service::checkConfig(const ConfigSection& section, const std::string& filename)
{
  bool hasLocalhop = false;
  bool hasPropagate = false;

  for (const auto& item : section) {
    const std::string& key = item.first;
    const ConfigSection& value = item.second;
    if (key == CFG_LOCALHOST_SECURITY || key == CFG_PA_VALIDATION) {
      ndn::ValidatorConfig testValidator(m_face);
      testValidator.load(value, filename);
    }
    else if (key == CFG_LOCALHOP_SECURITY) {
      hasLocalhop = true;
      ndn::ValidatorConfig testValidator(m_face);
      testValidator.load(value, filename);
    }
    else if (key == CFG_PREFIX_PROPAGATE) {
      hasPropagate = true;
      // AutoPrefixPropagator does not support config dry-run
    }
    else if (key == CFG_READVERTISE_NLSR) {
      ConfigFile::parseYesNo(item, CFG_RIB + "." + CFG_READVERTISE_NLSR);
    }
    else {
      NDN_THROW(ConfigFile::Error("Unrecognized option " + CFG_RIB + "." + key));
    }
  }

  if (hasLocalhop && hasPropagate) {
    NDN_THROW(ConfigFile::Error(CFG_LOCALHOP_SECURITY + " and " + CFG_PREFIX_PROPAGATE +
                                " cannot be enabled at the same time"));
  }
}

void
Service::applyConfig(const ConfigSection& section, const std::string& filename)
{
  bool wantPrefixPropagate = false;
  bool wantReadvertiseNlsr = false;

  for (const auto& item : section) {
    const std::string& key = item.first;
    const ConfigSection& value = item.second;
    if (key == CFG_LOCALHOST_SECURITY) {
      m_ribManager.applyLocalhostConfig(value, filename);
    }
    else if (key == CFG_LOCALHOP_SECURITY) {
      m_ribManager.enableLocalhop(value, filename);
    }
    else if (key == CFG_PA_VALIDATION) {
      m_ribManager.applyPaConfig(value, filename);
    }
    else if (key == CFG_PREFIX_PROPAGATE) {
      wantPrefixPropagate = true;

      if (!m_readvertisePropagation) {
        NFD_LOG_DEBUG("Enabling automatic prefix propagation");

        auto cost = item.second.get_optional<uint64_t>("cost");
        auto parameters = ndn::nfd::ControlParameters()
                          .setCost(cost.value_or(PROPAGATE_DEFAULT_COST))
                          .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT);

        auto timeout = item.second.get_optional<uint64_t>("timeout");
        auto options = ndn::nfd::CommandOptions()
                       .setPrefix(RibManager::LOCALHOP_TOP_PREFIX)
                       .setTimeout(timeout ? time::milliseconds(*timeout) : PROPAGATE_DEFAULT_TIMEOUT);

        m_readvertisePropagation = make_unique<Readvertise>(
          m_rib,
          make_unique<HostToGatewayReadvertisePolicy>(m_keyChain, item.second),
          make_unique<NfdRibReadvertiseDestination>(m_nfdController, m_rib, options, parameters));
      }
    }
    else if (key == CFG_READVERTISE_NLSR) {
      wantReadvertiseNlsr = ConfigFile::parseYesNo(item, CFG_RIB + "." + CFG_READVERTISE_NLSR);
    }
    else {
      NDN_THROW(ConfigFile::Error("Unrecognized option " + CFG_RIB + "." + key));
    }
  }

  if (!wantPrefixPropagate && m_readvertisePropagation != nullptr) {
    NFD_LOG_DEBUG("Disabling automatic prefix propagation");
    m_readvertisePropagation.reset();
  }

  if (wantReadvertiseNlsr && m_readvertiseNlsr == nullptr) {
    NFD_LOG_DEBUG("Enabling readvertise-to-nlsr");
    auto options = ndn::nfd::CommandOptions().setPrefix(READVERTISE_NLSR_PREFIX);
    m_readvertiseNlsr = make_unique<Readvertise>(
      m_rib,
      make_unique<ClientToNlsrReadvertisePolicy>(),
      make_unique<NfdRibReadvertiseDestination>(m_nfdController, m_rib, options));
  }
  else if (!wantReadvertiseNlsr && m_readvertiseNlsr != nullptr) {
    NFD_LOG_DEBUG("Disabling readvertise-to-nlsr");
    m_readvertiseNlsr.reset();
  }
}

} // namespace rib
} // namespace nfd
