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

#include "common/global.hpp"

namespace nfd {

static thread_local unique_ptr<Scheduler> g_scheduler;

namespace detail {

void
SimulatorIo::post(const std::function<void()>& callback)
{
  getScheduler().schedule(0_s, callback);
}

void
SimulatorIo::dispatch(const std::function<void()>& callback)
{
  getScheduler().schedule(0_s, callback);
}

} // namespace detail

detail::SimulatorIo&
getGlobalIoService()
{
  static detail::SimulatorIo io;
  return io;
}

detail::SimulatorIo&
getMainIoService()
{
  static detail::SimulatorIo io;
  return io;
}

detail::SimulatorIo&
getRibIoService()
{
  static detail::SimulatorIo io;
  return io;
}

Scheduler&
getScheduler()
{
  if (g_scheduler == nullptr) {
    static ndn::DummyIoService io;
    g_scheduler = make_unique<Scheduler>(io);
  }
  return *g_scheduler;
}

void
runOnMainIoService(const std::function<void()>& f)
{
  getMainIoService().post(f);
}

void
runOnRibIoService(const std::function<void()>& f)
{
  getRibIoService().post(f);
}

void
resetGlobalScheduler()
{
  g_scheduler.reset();
}

} // namespace nfd
