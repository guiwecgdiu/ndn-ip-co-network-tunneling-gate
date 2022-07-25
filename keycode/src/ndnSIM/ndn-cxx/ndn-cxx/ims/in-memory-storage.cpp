/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
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

#include "ndn-cxx/ims/in-memory-storage.hpp"
#include "ndn-cxx/ims/in-memory-storage-entry.hpp"

namespace ndn {

const time::milliseconds InMemoryStorage::INFINITE_WINDOW(-1);
const time::milliseconds InMemoryStorage::ZERO_WINDOW(0);

InMemoryStorage::const_iterator::const_iterator(const Data* ptr, const Cache* cache,
                                                Cache::index<byFullName>::type::iterator it)
  : m_ptr(ptr)
  , m_cache(cache)
  , m_it(it)
{
}

InMemoryStorage::const_iterator&
InMemoryStorage::const_iterator::operator++()
{
  m_it++;
  if (m_it != m_cache->get<byFullName>().end()) {
    m_ptr = &((*m_it)->getData());
  }
  else {
    m_ptr = nullptr;
  }

  return *this;
}

InMemoryStorage::const_iterator
InMemoryStorage::const_iterator::operator++(int)
{
  InMemoryStorage::const_iterator i(*this);
  this->operator++();
  return i;
}

InMemoryStorage::const_iterator::reference
InMemoryStorage::const_iterator::operator*()
{
  return *m_ptr;
}

InMemoryStorage::const_iterator::pointer
InMemoryStorage::const_iterator::operator->()
{
  return m_ptr;
}

bool
InMemoryStorage::const_iterator::operator==(const const_iterator& rhs)
{
  return m_it == rhs.m_it;
}

bool
InMemoryStorage::const_iterator::operator!=(const const_iterator& rhs)
{
  return m_it != rhs.m_it;
}

InMemoryStorage::InMemoryStorage(size_t limit)
  : m_limit(limit)
  , m_nPackets(0)
{
  init();
}

InMemoryStorage::InMemoryStorage(DummyIoService& ioService, size_t limit)
  : m_limit(limit)
  , m_nPackets(0)
{
  m_scheduler = make_unique<Scheduler>(ioService);
  init();
}

void
InMemoryStorage::init()
{
  // TODO consider a more suitable initial value
  m_capacity = m_initCapacity;

  if (m_limit != std::numeric_limits<size_t>::max() && m_capacity > m_limit) {
    m_capacity = m_limit;
  }

  for (size_t i = 0; i < m_capacity; i++) {
    m_freeEntries.push(new InMemoryStorageEntry());
  }
}

InMemoryStorage::~InMemoryStorage()
{
  // evict all items from cache
  auto it = m_cache.begin();
  while (it != m_cache.end()) {
    it = freeEntry(it);
  }

  BOOST_ASSERT(m_freeEntries.size() == m_capacity);

  while (!m_freeEntries.empty()) {
    delete m_freeEntries.top();
    m_freeEntries.pop();
  }
}

void
InMemoryStorage::setCapacity(size_t capacity)
{
  size_t oldCapacity = m_capacity;
  m_capacity = std::max(capacity, m_initCapacity);

  if (size() > m_capacity) {
    ssize_t nAllowedFailures = size() - m_capacity;
    while (size() > m_capacity) {
      if (!evictItem() && --nAllowedFailures < 0) {
        NDN_THROW(Error());
      }
    }
  }

  if (m_capacity >= oldCapacity) {
    for (size_t i = oldCapacity; i < m_capacity; i++) {
      m_freeEntries.push(new InMemoryStorageEntry());
    }
  }
  else {
    for (size_t i = oldCapacity; i > m_capacity; i--) {
      delete m_freeEntries.top();
      m_freeEntries.pop();
    }
  }

  BOOST_ASSERT(size() + m_freeEntries.size() == m_capacity);
}

void
InMemoryStorage::insert(const Data& data, const time::milliseconds& mustBeFreshProcessingWindow)
{
  // check if identical Data/Name already exists
  auto it = m_cache.get<byFullName>().find(data.getFullName());
  if (it != m_cache.get<byFullName>().end())
    return;

  //if full, double the capacity
  bool doesReachLimit = (getLimit() == getCapacity());
  if (isFull() && !doesReachLimit) {
    // note: This is incorrect if 2*capacity overflows, but memory should run out before that
    size_t newCapacity = std::min(2 * getCapacity(), getLimit());
    setCapacity(newCapacity);
  }

  //if full and reach limitation of the capacity, employ replacement policy
  if (isFull() && doesReachLimit) {
    evictItem();
  }

  //insert to cache
  BOOST_ASSERT(m_freeEntries.size() > 0);
  // take entry for the memory pool
  InMemoryStorageEntry* entry = m_freeEntries.top();
  m_freeEntries.pop();
  m_nPackets++;
  entry->setData(data);
  if (m_scheduler != nullptr && mustBeFreshProcessingWindow > ZERO_WINDOW) {
    entry->scheduleMarkStale(*m_scheduler, mustBeFreshProcessingWindow);
  }
  m_cache.insert(entry);

  //let derived class do something with the entry
  afterInsert(entry);
}

shared_ptr<const Data>
InMemoryStorage::find(const Name& name)
{
  auto it = m_cache.get<byFullName>().lower_bound(name);

  // if not found, return null
  if (it == m_cache.get<byFullName>().end()) {
    return nullptr;
  }

  // if the given name is not the prefix of the lower_bound, return null
  if (!name.isPrefixOf((*it)->getFullName())) {
    return nullptr;
  }

  afterAccess(*it);
  return ((*it)->getData()).shared_from_this();
}

shared_ptr<const Data>
InMemoryStorage::find(const Interest& interest)
{
  // if the interest contains implicit digest, it is possible to directly locate a packet.
  auto it = m_cache.get<byFullName>().find(interest.getName());

  // if a packet is located by its full name, it must be the packet to return.
  if (it != m_cache.get<byFullName>().end()) {
    return ((*it)->getData()).shared_from_this();
  }

  // if the packet is not discovered by last step, either the packet is not in the storage or
  // the interest doesn't contains implicit digest.
  it = m_cache.get<byFullName>().lower_bound(interest.getName());

  if (it == m_cache.get<byFullName>().end()) {
    return nullptr;
  }

  // to locate the element that has a just smaller name than the interest's
  if (it != m_cache.get<byFullName>().begin()) {
    it--;
  }

  InMemoryStorageEntry* ret = selectChild(interest, it);
  if (ret == nullptr) {
    return nullptr;
  }

  // let derived class do something with the entry
  afterAccess(ret);
  return ret->getData().shared_from_this();
}

InMemoryStorage::Cache::index<InMemoryStorage::byFullName>::type::iterator
InMemoryStorage::findNextFresh(Cache::index<byFullName>::type::iterator it) const
{
  for (; it != m_cache.get<byFullName>().end(); it++) {
    if ((*it)->isFresh())
      return it;
  }

  return it;
}

InMemoryStorageEntry*
InMemoryStorage::selectChild(const Interest& interest,
                             Cache::index<byFullName>::type::iterator startingPoint) const
{
  BOOST_ASSERT(startingPoint != m_cache.get<byFullName>().end());

  if (startingPoint != m_cache.get<byFullName>().begin()) {
    BOOST_ASSERT((*startingPoint)->getFullName() < interest.getName());
  }

  // filter out non-fresh data
  if (interest.getMustBeFresh()) {
    startingPoint = findNextFresh(startingPoint);
  }

  if (startingPoint == m_cache.get<byFullName>().end()) {
    return nullptr;
  }

  if (interest.matchesData((*startingPoint)->getData())) {
    return *startingPoint;
  }

  // iterate to the right
  auto rightmostCandidate = startingPoint;
  while (true) {
    ++rightmostCandidate;
    // filter out non-fresh data
    if (interest.getMustBeFresh()) {
      rightmostCandidate = findNextFresh(rightmostCandidate);
    }

    bool isInPrefix = false;
    if (rightmostCandidate != m_cache.get<byFullName>().end()) {
      isInPrefix = interest.getName().isPrefixOf((*rightmostCandidate)->getFullName());
    }
    if (isInPrefix) {
      if (interest.matchesData((*rightmostCandidate)->getData())) {
        return *rightmostCandidate;
      }
    }
    else {
      break;
    }
  }

  return nullptr;
}

InMemoryStorage::Cache::iterator
InMemoryStorage::freeEntry(Cache::iterator it)
{
  // push the *empty* entry into mem pool
  (*it)->release();
  m_freeEntries.push(*it);
  m_nPackets--;
  return m_cache.erase(it);
}

void
InMemoryStorage::erase(const Name& prefix, const bool isPrefix)
{
  if (isPrefix) {
    auto it = m_cache.get<byFullName>().lower_bound(prefix);
    while (it != m_cache.get<byFullName>().end() && prefix.isPrefixOf((*it)->getName())) {
      // let derived class do something with the entry
      beforeErase(*it);
      it = freeEntry(it);
    }
  }
  else {
    auto it = m_cache.get<byFullName>().find(prefix);
    if (it == m_cache.get<byFullName>().end())
      return;

    // let derived class do something with the entry
    beforeErase(*it);
    freeEntry(it);
  }

  if (m_freeEntries.size() > (2 * size()))
    setCapacity(getCapacity() / 2);
}

void
InMemoryStorage::eraseImpl(const Name& name)
{
  auto it = m_cache.get<byFullName>().find(name);
  if (it == m_cache.get<byFullName>().end())
    return;

  freeEntry(it);
}

InMemoryStorage::const_iterator
InMemoryStorage::begin() const
{
  auto it = m_cache.get<byFullName>().begin();
  return const_iterator(&((*it)->getData()), &m_cache, it);
}

InMemoryStorage::const_iterator
InMemoryStorage::end() const
{
  auto it = m_cache.get<byFullName>().end();
  return const_iterator(nullptr, &m_cache, it);
}

void
InMemoryStorage::afterInsert(InMemoryStorageEntry* entry)
{
}

void
InMemoryStorage::beforeErase(InMemoryStorageEntry* entry)
{
}

void
InMemoryStorage::afterAccess(InMemoryStorageEntry* entry)
{
}

void
InMemoryStorage::printCache(std::ostream& os) const
{
  // start from the upper layer towards bottom
  for (const auto& elem : m_cache.get<byFullName>())
    os << elem->getFullName() << std::endl;
}

} // namespace ndn
