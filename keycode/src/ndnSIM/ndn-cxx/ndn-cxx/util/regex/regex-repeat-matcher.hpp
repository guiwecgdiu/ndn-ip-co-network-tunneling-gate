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
 *
 * @author Yingdi Yu <http://irl.cs.ucla.edu/~yingdi/>
 */

#ifndef NDN_CXX_UTIL_REGEX_REGEX_REPEAT_MATCHER_HPP
#define NDN_CXX_UTIL_REGEX_REGEX_REPEAT_MATCHER_HPP

#include "ndn-cxx/util/regex/regex-matcher.hpp"

namespace ndn {

class RegexRepeatMatcher : public RegexMatcher
{
public:
  RegexRepeatMatcher(const std::string& expr,
                     shared_ptr<RegexBackrefManager> backrefManager,
                     size_t indicator);

  bool
  match(const Name& name, size_t offset, size_t len) override;

private:
  void
  compile();

  void
  parseRepetition();

  bool
  recursiveMatch(size_t repeat, const Name& name, size_t offset, size_t len);

private:
  size_t m_indicator;
  size_t m_repeatMin = 0;
  size_t m_repeatMax = 0;
};

} // namespace ndn

#endif // NDN_CXX_UTIL_REGEX_REGEX_REPEAT_MATCHER_HPP
