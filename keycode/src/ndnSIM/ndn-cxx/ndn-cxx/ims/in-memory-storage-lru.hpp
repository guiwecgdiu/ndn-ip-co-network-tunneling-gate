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

#ifndef NDN_CXX_IMS_IN_MEMORY_STORAGE_LRU_HPP
#define NDN_CXX_IMS_IN_MEMORY_STORAGE_LRU_HPP

#include "ndn-cxx/ims/in-memory-storage.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>

namespace ndn {

/** @brief Provides in-memory storage employing Least Recently Used (LRU) replacement policy.
 */
class InMemoryStorageLru : public InMemoryStorage
{
public:
  explicit
  InMemoryStorageLru(size_t limit = 16);

  InMemoryStorageLru(DummyIoService& ioService, size_t limit = 16);

NDN_CXX_PUBLIC_WITH_TESTS_ELSE_PROTECTED:
  /** @brief Removes one Data packet from in-memory storage based on LRU, i.e. evict the least
   *  recently accessed Data packet
   *  @return{ whether the Data was removed }
   */
  bool
  evictItem() override;

  /** @brief Update the entry when the entry is returned by the find() function,
   *  update the last used time according to LRU
   */
  void
  afterAccess(InMemoryStorageEntry* entry) override;

  /** @brief Update the entry after a entry is successfully inserted, add it to the cleanupIndex
   */
  void
  afterInsert(InMemoryStorageEntry* entry) override;

  /** @brief Update the entry or other data structures before a entry is successfully erased,
   *  erase it from the cleanupIndex
   */
  void
  beforeErase(InMemoryStorageEntry* entry) override;

private:
  // multi_index_container to implement LRU
  class byUsedTime;
  class byEntity;

  typedef boost::multi_index_container<
    InMemoryStorageEntry*,
    boost::multi_index::indexed_by<

      // by Entry itself
      boost::multi_index::hashed_unique<
        boost::multi_index::tag<byEntity>,
        boost::multi_index::identity<InMemoryStorageEntry*>
      >,

      // by last used time (LRU)
      boost::multi_index::sequenced<
        boost::multi_index::tag<byUsedTime>
      >

    >
  > CleanupIndex;

  CleanupIndex m_cleanupIndex;
};

} // namespace ndn

#endif // NDN_CXX_IMS_IN_MEMORY_STORAGE_LRU_HPP
