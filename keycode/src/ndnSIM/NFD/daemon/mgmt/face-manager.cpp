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

#include "face-manager.hpp"

#include "common/logger.hpp"
#include "face/generic-link-service.hpp"
#include "face/protocol-factory.hpp"
#include "fw/face-table.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/channel-status.hpp>
#include <ndn-cxx/mgmt/nfd/face-event-notification.hpp>
#include <ndn-cxx/mgmt/nfd/face-query-filter.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>

namespace nfd {

NFD_LOG_INIT(FaceManager);

FaceManager::FaceManager(FaceSystem& faceSystem,
                         Dispatcher& dispatcher, CommandAuthenticator& authenticator)
  : ManagerBase("faces", dispatcher, authenticator)
  , m_faceSystem(faceSystem)
  , m_faceTable(faceSystem.getFaceTable())
{
  // register handlers for ControlCommand
  registerCommandHandler<ndn::nfd::FaceCreateCommand>("create",
    std::bind(&FaceManager::createFace, this, _4, _5));
  registerCommandHandler<ndn::nfd::FaceUpdateCommand>("update",
    std::bind(&FaceManager::updateFace, this, _3, _4, _5));
  registerCommandHandler<ndn::nfd::FaceDestroyCommand>("destroy",
    std::bind(&FaceManager::destroyFace, this, _4, _5));

  // register handlers for StatusDataset
  registerStatusDatasetHandler("list", std::bind(&FaceManager::listFaces, this, _3));
  registerStatusDatasetHandler("channels", std::bind(&FaceManager::listChannels, this, _3));
  registerStatusDatasetHandler("query", std::bind(&FaceManager::queryFaces, this, _2, _3));

  // register notification stream
  m_postNotification = registerNotificationStream("events");
  m_faceAddConn = m_faceTable.afterAdd.connect([this] (const Face& face) {
    connectFaceStateChangeSignal(face);
    notifyFaceEvent(face, ndn::nfd::FACE_EVENT_CREATED);
  });
  m_faceRemoveConn = m_faceTable.beforeRemove.connect([this] (const Face& face) {
    notifyFaceEvent(face, ndn::nfd::FACE_EVENT_DESTROYED);
  });
}

void
FaceManager::createFace(const ControlParameters& parameters,
                        const ndn::mgmt::CommandContinuation& done)
{
  FaceUri remoteUri;
  if (!remoteUri.parse(parameters.getUri())) {
    NFD_LOG_TRACE("failed to parse remote URI: " << parameters.getUri());
    done(ControlResponse(400, "Malformed command"));
    return;
  }

  if (!remoteUri.isCanonical()) {
    NFD_LOG_TRACE("received non-canonical remote URI: " << remoteUri.toString());
    done(ControlResponse(400, "Non-canonical remote URI"));
    return;
  }

  optional<FaceUri> localUri;
  if (parameters.hasLocalUri()) {
    localUri = FaceUri{};

    if (!localUri->parse(parameters.getLocalUri())) {
      NFD_LOG_TRACE("failed to parse local URI: " << parameters.getLocalUri());
      done(ControlResponse(400, "Malformed command"));
      return;
    }

    if (!localUri->isCanonical()) {
      NFD_LOG_TRACE("received non-canonical local URI: " << localUri->toString());
      done(ControlResponse(400, "Non-canonical local URI"));
      return;
    }
  }

  face::ProtocolFactory* factory = m_faceSystem.getFactoryByScheme(remoteUri.getScheme());
  if (factory == nullptr) {
    NFD_LOG_TRACE("received create request for unsupported protocol: " << remoteUri.getScheme());
    done(ControlResponse(406, "Unsupported protocol"));
    return;
  }

  face::FaceParams faceParams;
  faceParams.persistency = parameters.getFacePersistency();
  if (parameters.hasBaseCongestionMarkingInterval()) {
    faceParams.baseCongestionMarkingInterval = parameters.getBaseCongestionMarkingInterval();
  }
  if (parameters.hasDefaultCongestionThreshold()) {
    faceParams.defaultCongestionThreshold = parameters.getDefaultCongestionThreshold();
  }
  if (parameters.hasMtu()) {
    // The face system limits MTUs to ssize_t, but the management protocol uses uint64_t
    faceParams.mtu = std::min<uint64_t>(std::numeric_limits<ssize_t>::max(), parameters.getMtu());
  }
  faceParams.wantLocalFields = parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
                               parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED);
  faceParams.wantLpReliability = parameters.hasFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED) &&
                                 parameters.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED);
  if (parameters.hasFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) {
    faceParams.wantCongestionMarking = parameters.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED);
  }
  try {
    factory->createFace({remoteUri, localUri, faceParams},
                        [this, parameters, done] (const auto& face) {
                          this->afterCreateFaceSuccess(face, parameters, done);
                        },
                        [done] (uint32_t status, const std::string& reason) {
                          NFD_LOG_DEBUG("Face creation failed: " << reason);
                          done(ControlResponse(status, reason));
                        });
  }
  catch (const std::runtime_error& error) {
    NFD_LOG_ERROR("Face creation failed: " << error.what());
    done(ControlResponse(500, "Face creation failed due to internal error"));
    return;
  }
  catch (const std::logic_error& error) {
    NFD_LOG_ERROR("Face creation failed: " << error.what());
    done(ControlResponse(500, "Face creation failed due to internal error"));
    return;
  }
}

template<typename T>
static void
copyMtu(const Face& face, T& to)
{
  if (face.getMtu() >= 0) {
    to.setMtu(std::min<size_t>(face.getMtu(), ndn::MAX_NDN_PACKET_SIZE));
  }
  else if (face.getMtu() == face::MTU_UNLIMITED) {
    to.setMtu(ndn::MAX_NDN_PACKET_SIZE);
  }
}

static ControlParameters
makeUpdateFaceResponse(const Face& face)
{
  ControlParameters params;
  params.setFaceId(face.getId())
        .setFacePersistency(face.getPersistency());
  copyMtu(face, params);

  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  if (linkService != nullptr) {
    const auto& options = linkService->getOptions();
    params.setBaseCongestionMarkingInterval(options.baseCongestionMarkingInterval)
          .setDefaultCongestionThreshold(options.defaultCongestionThreshold)
          .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, options.allowLocalFields, false)
          .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, options.reliabilityOptions.isEnabled, false)
          .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, options.allowCongestionMarking, false);
  }

  return params;
}

static ControlParameters
makeCreateFaceResponse(const Face& face)
{
  ControlParameters params = makeUpdateFaceResponse(face);
  params.setUri(face.getRemoteUri().toString())
        .setLocalUri(face.getLocalUri().toString());

  return params;
}

void
FaceManager::afterCreateFaceSuccess(const shared_ptr<Face>& face,
                                    const ControlParameters& parameters,
                                    const ndn::mgmt::CommandContinuation& done)
{
  if (face->getId() != face::INVALID_FACEID) { // Face already exists
    NFD_LOG_TRACE("Attempted to create duplicate face of " << face->getId());
    ControlParameters response = makeCreateFaceResponse(*face);
    done(ControlResponse(409, "Face with remote URI already exists").setBody(response.wireEncode()));
    return;
  }

  // If scope non-local and flags set to enable local fields, request shouldn't
  // have made it this far
  BOOST_ASSERT(face->getScope() == ndn::nfd::FACE_SCOPE_LOCAL ||
               !parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) ||
               (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
                !parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED)));

  m_faceTable.add(face);

  ControlParameters response = makeCreateFaceResponse(*face);
  done(ControlResponse(200, "OK").setBody(response.wireEncode()));
}

static void
updateLinkServiceOptions(Face& face, const ControlParameters& parameters)
{
  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  if (linkService == nullptr) {
    return;
  }
  auto options = linkService->getOptions();

  if (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      face.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
    options.allowLocalFields = parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED);
  }
  if (parameters.hasFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED)) {
    options.reliabilityOptions.isEnabled = parameters.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED);
  }
  if (parameters.hasFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) {
    options.allowCongestionMarking = parameters.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED);
  }
  if (parameters.hasBaseCongestionMarkingInterval()) {
    options.baseCongestionMarkingInterval = parameters.getBaseCongestionMarkingInterval();
  }
  if (parameters.hasDefaultCongestionThreshold()) {
    options.defaultCongestionThreshold = parameters.getDefaultCongestionThreshold();
  }

  if (parameters.hasMtu()) {
    // The face system limits MTUs to ssize_t, but the management protocol uses uint64_t
    options.overrideMtu = std::min<uint64_t>(std::numeric_limits<ssize_t>::max(), parameters.getMtu());
  }

  linkService->setOptions(options);
}

void
FaceManager::updateFace(const Interest& interest,
                        const ControlParameters& parameters,
                        const ndn::mgmt::CommandContinuation& done)
{
  FaceId faceId = parameters.getFaceId();
  if (faceId == 0) { // Self-update
    auto incomingFaceIdTag = interest.getTag<lp::IncomingFaceIdTag>();
    if (incomingFaceIdTag == nullptr) {
      NFD_LOG_TRACE("unable to determine face for self-update");
      done(ControlResponse(404, "No FaceId specified and IncomingFaceId not available"));
      return;
    }
    faceId = *incomingFaceIdTag;
  }

  Face* face = m_faceTable.get(faceId);
  if (face == nullptr) {
    NFD_LOG_TRACE("invalid face specified");
    done(ControlResponse(404, "Specified face does not exist"));
    return;
  }

  // Verify validity of requested changes
  ControlParameters response;
  bool areParamsValid = true;

  if (parameters.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED) &&
      face->getScope() != ndn::nfd::FACE_SCOPE_LOCAL) {
    NFD_LOG_TRACE("received request to enable local fields on non-local face");
    areParamsValid = false;
    response.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED,
                        parameters.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
  }

  // check whether the requested FacePersistency change is valid if it's present
  if (parameters.hasFacePersistency()) {
    auto persistency = parameters.getFacePersistency();
    if (!face->getTransport()->canChangePersistencyTo(persistency)) {
      NFD_LOG_TRACE("cannot change face persistency to " << persistency);
      areParamsValid = false;
      response.setFacePersistency(persistency);
    }
  }

  // check whether the requested MTU override is valid (if it's present)
  if (parameters.hasMtu()) {
    auto mtu = parameters.getMtu();
    // The face system limits MTUs to ssize_t, but the management protocol uses uint64_t
    auto actualMtu = std::min<uint64_t>(std::numeric_limits<ssize_t>::max(), mtu);
    auto linkService = dynamic_cast<face::GenericLinkService*>(face->getLinkService());
    if (linkService == nullptr || !linkService->canOverrideMtuTo(actualMtu)) {
      NFD_LOG_TRACE("cannot override face MTU to " << mtu);
      areParamsValid = false;
      response.setMtu(mtu);
    }
  }

  if (!areParamsValid) {
    done(ControlResponse(409, "Invalid properties specified").setBody(response.wireEncode()));
    return;
  }

  // All specified properties are valid, so make changes
  if (parameters.hasFacePersistency()) {
    face->setPersistency(parameters.getFacePersistency());
  }
  updateLinkServiceOptions(*face, parameters);

  // Prepare and send ControlResponse
  response = makeUpdateFaceResponse(*face);
  done(ControlResponse(200, "OK").setBody(response.wireEncode()));
}

void
FaceManager::destroyFace(const ControlParameters& parameters,
                         const ndn::mgmt::CommandContinuation& done)
{
  Face* face = m_faceTable.get(parameters.getFaceId());
  if (face != nullptr) {
    face->close();
  }

  done(ControlResponse(200, "OK").setBody(parameters.wireEncode()));
}

template<typename T>
static void
copyFaceProperties(const Face& face, T& to)
{
  to.setFaceId(face.getId())
    .setRemoteUri(face.getRemoteUri().toString())
    .setLocalUri(face.getLocalUri().toString())
    .setFaceScope(face.getScope())
    .setFacePersistency(face.getPersistency())
    .setLinkType(face.getLinkType());

  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  if (linkService != nullptr) {
    const auto& options = linkService->getOptions();
    to.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, options.allowLocalFields)
      .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, options.reliabilityOptions.isEnabled)
      .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, options.allowCongestionMarking);
  }
}

static ndn::nfd::FaceStatus
makeFaceStatus(const Face& face, const time::steady_clock::time_point& now)
{
  ndn::nfd::FaceStatus status;
  copyFaceProperties(face, status);

  auto expirationTime = face.getExpirationTime();
  if (expirationTime != time::steady_clock::time_point::max()) {
    status.setExpirationPeriod(std::max(0_ms,
                                        time::duration_cast<time::milliseconds>(expirationTime - now)));
  }

  auto linkService = dynamic_cast<face::GenericLinkService*>(face.getLinkService());
  if (linkService != nullptr) {
    const auto& options = linkService->getOptions();
    status.setBaseCongestionMarkingInterval(options.baseCongestionMarkingInterval)
          .setDefaultCongestionThreshold(options.defaultCongestionThreshold);
  }

  copyMtu(face, status);

  const auto& counters = face.getCounters();
  status.setNInInterests(counters.nInInterests)
        .setNOutInterests(counters.nOutInterests)
        .setNInData(counters.nInData)
        .setNOutData(counters.nOutData)
        .setNInNacks(counters.nInNacks)
        .setNOutNacks(counters.nOutNacks)
        .setNInBytes(counters.nInBytes)
        .setNOutBytes(counters.nOutBytes);

  return status;
}

void
FaceManager::listFaces(ndn::mgmt::StatusDatasetContext& context)
{
  auto now = time::steady_clock::now();
  for (const auto& face : m_faceTable) {
    ndn::nfd::FaceStatus status = makeFaceStatus(face, now);
    context.append(status.wireEncode());
  }
  context.end();
}

void
FaceManager::listChannels(ndn::mgmt::StatusDatasetContext& context)
{
  auto factories = m_faceSystem.listProtocolFactories();
  for (const auto* factory : factories) {
    for (const auto& channel : factory->getChannels()) {
      ndn::nfd::ChannelStatus entry;
      entry.setLocalUri(channel->getUri().toString());
      context.append(entry.wireEncode());
    }
  }
  context.end();
}

static bool
matchFilter(const ndn::nfd::FaceQueryFilter& filter, const Face& face)
{
  if (filter.hasFaceId() &&
      filter.getFaceId() != static_cast<uint64_t>(face.getId())) {
    return false;
  }

  if (filter.hasUriScheme() &&
      filter.getUriScheme() != face.getRemoteUri().getScheme() &&
      filter.getUriScheme() != face.getLocalUri().getScheme()) {
    return false;
  }

  if (filter.hasRemoteUri() &&
      filter.getRemoteUri() != face.getRemoteUri().toString()) {
    return false;
  }

  if (filter.hasLocalUri() &&
      filter.getLocalUri() != face.getLocalUri().toString()) {
    return false;
  }

  if (filter.hasFaceScope() &&
      filter.getFaceScope() != face.getScope()) {
    return false;
  }

  if (filter.hasFacePersistency() &&
      filter.getFacePersistency() != face.getPersistency()) {
    return false;
  }

  if (filter.hasLinkType() &&
      filter.getLinkType() != face.getLinkType()) {
    return false;
  }

  return true;
}

void
FaceManager::queryFaces(const Interest& interest,
                        ndn::mgmt::StatusDatasetContext& context)
{
  ndn::nfd::FaceQueryFilter faceFilter;
  try {
    faceFilter.wireDecode(interest.getName()[-1].blockFromValue());
  }
  catch (const tlv::Error& e) {
    NFD_LOG_DEBUG("Malformed query filter: " << e.what());
    return context.reject(ControlResponse(400, "Malformed filter"));
  }

  auto now = time::steady_clock::now();
  for (const auto& face : m_faceTable) {
    if (matchFilter(faceFilter, face)) {
      ndn::nfd::FaceStatus status = makeFaceStatus(face, now);
      context.append(status.wireEncode());
    }
  }
  context.end();
}

void
FaceManager::notifyFaceEvent(const Face& face, ndn::nfd::FaceEventKind kind)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(kind);
  copyFaceProperties(face, notification);

  m_postNotification(notification.wireEncode());
}

void
FaceManager::connectFaceStateChangeSignal(const Face& face)
{
  using face::FaceState;

  FaceId faceId = face.getId();
  m_faceStateChangeConn[faceId] = face.afterStateChange.connect(
    [this, faceId, &face] (FaceState oldState, FaceState newState) {
      if (newState == FaceState::UP) {
        notifyFaceEvent(face, ndn::nfd::FACE_EVENT_UP);
      }
      else if (newState == FaceState::DOWN) {
        notifyFaceEvent(face, ndn::nfd::FACE_EVENT_DOWN);
      }
      else if (newState == FaceState::CLOSED) {
        // cannot use face.getId() because it may already be reset to INVALID_FACEID
        m_faceStateChangeConn.erase(faceId);
      }
    });
}

} // namespace nfd
