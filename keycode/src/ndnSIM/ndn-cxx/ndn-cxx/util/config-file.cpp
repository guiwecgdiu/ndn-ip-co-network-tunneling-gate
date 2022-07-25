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

#include "ndn-cxx/util/config-file.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/property_tree/ini_parser.hpp>

namespace ndn {

ConfigFile::ConfigFile()
  : m_path(findConfigFile())
{
  if (open()) {
    parse();
    close();
  }
}

ConfigFile::~ConfigFile()
{
  close();
}

boost::filesystem::path
ConfigFile::findConfigFile()
{
  using namespace boost::filesystem;

#ifdef NDN_CXX_HAVE_TESTS
  if (std::getenv("TEST_HOME")) {
    path testHome(std::getenv("TEST_HOME"));
    testHome /= ".ndn/client.conf";
    if (exists(testHome)) {
      return absolute(testHome);
    }
  }
#endif // NDN_CXX_HAVE_TESTS

  if (std::getenv("HOME")) {
    path home(std::getenv("HOME"));
    home /= ".ndn/client.conf";
    if (exists(home)) {
      return absolute(home);
    }
  }

#ifdef NDN_CXX_SYSCONFDIR
  path sysconfdir(NDN_CXX_SYSCONFDIR);
  sysconfdir /= "ndn/client.conf";
  if (exists(sysconfdir)) {
    return absolute(sysconfdir);
  }
#endif // NDN_CXX_SYSCONFDIR

  path etc("/etc/ndn/client.conf");
  if (exists(etc)) {
    return absolute(etc);
  }

  return {};
}

bool
ConfigFile::open()
{
  if (m_path.empty()) {
    return false;
  }

  m_input.open(m_path.c_str());
  if (!m_input.good() || !m_input.is_open()) {
    return false;
  }
  return true;
}

void
ConfigFile::close()
{
  if (m_input.is_open()) {
    m_input.close();
  }
}

const ConfigFile::Parsed&
ConfigFile::parse()
{
  if (m_path.empty()) {
    NDN_THROW(Error("Failed to locate configuration file for parsing"));
  }
  if (!m_input.is_open() && !open()) {
    NDN_THROW(Error("Failed to open configuration file for parsing"));
  }

  try {
    boost::property_tree::read_ini(m_input, m_config);
  }
  catch (const boost::property_tree::ini_parser_error& error) {
    NDN_THROW(Error("Failed to parse configuration file " + error.filename() +
                    " line " + to_string(error.line()) + ": " + error.message()));
  }
  return m_config;
}

} // namespace ndn
