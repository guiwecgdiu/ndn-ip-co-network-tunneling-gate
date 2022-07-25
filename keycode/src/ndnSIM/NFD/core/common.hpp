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

#ifndef NFD_CORE_COMMON_HPP
#define NFD_CORE_COMMON_HPP

#include "core/config.hpp"

#ifdef NFD_WITH_TESTS
#define NFD_VIRTUAL_WITH_TESTS virtual
#define NFD_PUBLIC_WITH_TESTS_ELSE_PROTECTED public
#define NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE public
#define NFD_PROTECTED_WITH_TESTS_ELSE_PRIVATE protected
#define NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL override
#define NFD_FINAL_UNLESS_WITH_TESTS
#else
#define NFD_VIRTUAL_WITH_TESTS
#define NFD_PUBLIC_WITH_TESTS_ELSE_PROTECTED protected
#define NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE private
#define NFD_PROTECTED_WITH_TESTS_ELSE_PRIVATE private
#define NFD_OVERRIDE_WITH_TESTS_ELSE_FINAL final
#define NFD_FINAL_UNLESS_WITH_TESTS final
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/net/face-uri.hpp>
#include <ndn-cxx/util/backports.hpp>
#include <ndn-cxx/util/exception.hpp>
#include <ndn-cxx/util/optional.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/signal.hpp>
#include <ndn-cxx/util/span.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

namespace nfd {

using std::size_t;

using boost::noncopyable;

using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
using std::make_shared;
using std::make_unique;

using std::static_pointer_cast;
using std::dynamic_pointer_cast;
using std::const_pointer_cast;

using namespace std::string_literals;

using ndn::optional;
using ndn::nullopt;
using ndn::span;
using ndn::to_string;

using ndn::Block;
using ndn::Data;
using ndn::FaceUri;
using ndn::Interest;
using ndn::Name;
using ndn::PartialName;
using ndn::Scheduler;

// Not using a namespace alias (namespace tlv = ndn::tlv), because
// it doesn't allow NFD to add other members to the namespace
namespace tlv {
using namespace ndn::tlv;
} // namespace tlv

namespace lp = ndn::lp;
namespace name = ndn::name;
namespace scheduler = ndn::scheduler;
namespace signal = ndn::util::signal;
namespace time = ndn::time;
using namespace ndn::time_literals;
using ndn::operator""_block;

} // namespace nfd

#endif // NFD_CORE_COMMON_HPP
