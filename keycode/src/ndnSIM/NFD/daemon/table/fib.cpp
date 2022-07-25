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

#include "fib.hpp"
#include "pit-entry.hpp"
#include "measurements-entry.hpp"

#include <ndn-cxx/util/concepts.hpp>

namespace nfd {
namespace fib {

NDN_CXX_ASSERT_FORWARD_ITERATOR(Fib::const_iterator);

const unique_ptr<Entry> Fib::s_emptyEntry = make_unique<Entry>(Name());

static inline bool
nteHasFibEntry(const name_tree::Entry& nte)
{
  return nte.getFibEntry() != nullptr;
}

Fib::Fib(NameTree& nameTree)
  : m_nameTree(nameTree)
{
}

template<typename K>
const Entry&
Fib::findLongestPrefixMatchImpl(const K& key) const
{
  name_tree::Entry* nte = m_nameTree.findLongestPrefixMatch(key, &nteHasFibEntry);
  if (nte != nullptr) {
    return *nte->getFibEntry();
  }
  return *s_emptyEntry;
}

const Entry&
Fib::findLongestPrefixMatch(const Name& prefix) const
{
  return this->findLongestPrefixMatchImpl(prefix);
}

const Entry&
Fib::findLongestPrefixMatch(const pit::Entry& pitEntry) const
{
  return this->findLongestPrefixMatchImpl(pitEntry);
}

const Entry&
Fib::findLongestPrefixMatch(const measurements::Entry& measurementsEntry) const
{
  return this->findLongestPrefixMatchImpl(measurementsEntry);
}

Entry*
Fib::findExactMatch(const Name& prefix)
{
  name_tree::Entry* nte = m_nameTree.findExactMatch(prefix);
  if (nte != nullptr)
    return nte->getFibEntry();

  return nullptr;
}

std::pair<Entry*, bool>
Fib::insert(const Name& prefix)
{
  name_tree::Entry& nte = m_nameTree.lookup(prefix);
  Entry* entry = nte.getFibEntry();
  if (entry != nullptr) {
    return {entry, false};
  }

  nte.setFibEntry(make_unique<Entry>(prefix));
  ++m_nItems;
  return {nte.getFibEntry(), true};
}

void
Fib::erase(name_tree::Entry* nte, bool canDeleteNte)
{
  BOOST_ASSERT(nte != nullptr);

  nte->setFibEntry(nullptr);
  if (canDeleteNte) {
    m_nameTree.eraseIfEmpty(nte);
  }
  --m_nItems;
}

void
Fib::erase(const Name& prefix)
{
  name_tree::Entry* nte = m_nameTree.findExactMatch(prefix);
  if (nte != nullptr) {
    this->erase(nte);
  }
}

void
Fib::erase(const Entry& entry)
{
  name_tree::Entry* nte = m_nameTree.getEntry(entry);
  if (nte == nullptr) { // don't try to erase s_emptyEntry
    BOOST_ASSERT(&entry == s_emptyEntry.get());
    return;
  }
  this->erase(nte);
}

void
Fib::addOrUpdateNextHop(Entry& entry, Face& face, uint64_t cost)
{
  NextHopList::iterator it;
  bool isNew;
  std::tie(it, isNew) = entry.addOrUpdateNextHop(face, cost);

  if (isNew)
    this->afterNewNextHop(entry.getPrefix(), *it);
}

Fib::RemoveNextHopResult
Fib::removeNextHop(Entry& entry, const Face& face)
{
  bool isRemoved = entry.removeNextHop(face);

  if (!isRemoved) {
    return RemoveNextHopResult::NO_SUCH_NEXTHOP;
  }
  else if (!entry.hasNextHops()) {
    name_tree::Entry* nte = m_nameTree.getEntry(entry);
    this->erase(nte, false);
    return RemoveNextHopResult::FIB_ENTRY_REMOVED;
  }
  else {
    return RemoveNextHopResult::NEXTHOP_REMOVED;
  }
}

Fib::Range
Fib::getRange() const
{
  return m_nameTree.fullEnumerate(&nteHasFibEntry) |
         boost::adaptors::transformed(name_tree::GetTableEntry<Entry>(&name_tree::Entry::getFibEntry));
}

} // namespace fib
} // namespace nfd
