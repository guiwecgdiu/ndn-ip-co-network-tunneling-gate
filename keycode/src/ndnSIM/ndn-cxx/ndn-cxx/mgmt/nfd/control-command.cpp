/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2022 Regents of the University of California.
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

#include "ndn-cxx/mgmt/nfd/control-command.hpp"

namespace ndn {
namespace nfd {

ControlCommand::ControlCommand(const std::string& module, const std::string& verb)
  : m_module(module)
  , m_verb(verb)
{
}

ControlCommand::~ControlCommand() = default;

void
ControlCommand::validateRequest(const ControlParameters& parameters) const
{
  m_requestValidator.validate(parameters);
}

void
ControlCommand::applyDefaultsToRequest(ControlParameters& parameters) const
{
}

void
ControlCommand::validateResponse(const ControlParameters& parameters) const
{
  m_responseValidator.validate(parameters);
}

void
ControlCommand::applyDefaultsToResponse(ControlParameters& parameters) const
{
}

Name
ControlCommand::getRequestName(const Name& commandPrefix,
                               const ControlParameters& parameters) const
{
  this->validateRequest(parameters);

  return Name(commandPrefix)
         .append(m_module)
         .append(m_verb)
         .append(parameters.wireEncode());
}

ControlCommand::FieldValidator::FieldValidator()
  : m_required(CONTROL_PARAMETER_UBOUND)
  , m_optional(CONTROL_PARAMETER_UBOUND)
{
}

void
ControlCommand::FieldValidator::validate(const ControlParameters& parameters) const
{
  const auto& presentFields = parameters.getPresentFields();

  for (size_t i = 0; i < CONTROL_PARAMETER_UBOUND; ++i) {
    bool isPresent = presentFields[i];
    if (m_required[i]) {
      if (!isPresent) {
        NDN_THROW(ArgumentError(CONTROL_PARAMETER_FIELD[i] + " is required but missing"));
      }
    }
    else if (isPresent && !m_optional[i]) {
      NDN_THROW(ArgumentError(CONTROL_PARAMETER_FIELD[i] + " is forbidden but present"));
    }
  }

  if (m_optional[CONTROL_PARAMETER_FLAGS] && m_optional[CONTROL_PARAMETER_MASK]) {
    if (parameters.hasFlags() != parameters.hasMask()) {
      NDN_THROW(ArgumentError("Flags must be accompanied by Mask"));
    }
  }
}

FaceCreateCommand::FaceCreateCommand()
  : ControlCommand("faces", "create")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_URI)
    .optional(CONTROL_PARAMETER_LOCAL_URI)
    .optional(CONTROL_PARAMETER_FACE_PERSISTENCY)
    .optional(CONTROL_PARAMETER_BASE_CONGESTION_MARKING_INTERVAL)
    .optional(CONTROL_PARAMETER_DEFAULT_CONGESTION_THRESHOLD)
    .optional(CONTROL_PARAMETER_MTU)
    .optional(CONTROL_PARAMETER_FLAGS)
    .optional(CONTROL_PARAMETER_MASK);
  m_responseValidator
    .required(CONTROL_PARAMETER_FACE_ID)
    .required(CONTROL_PARAMETER_URI)
    .required(CONTROL_PARAMETER_LOCAL_URI)
    .required(CONTROL_PARAMETER_FACE_PERSISTENCY)
    .optional(CONTROL_PARAMETER_BASE_CONGESTION_MARKING_INTERVAL)
    .optional(CONTROL_PARAMETER_DEFAULT_CONGESTION_THRESHOLD)
    .optional(CONTROL_PARAMETER_MTU)
    .required(CONTROL_PARAMETER_FLAGS);
}

void
FaceCreateCommand::applyDefaultsToRequest(ControlParameters& parameters) const
{
  if (!parameters.hasFacePersistency()) {
    parameters.setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT);
  }
}

void
FaceCreateCommand::validateResponse(const ControlParameters& parameters) const
{
  this->ControlCommand::validateResponse(parameters);

  if (parameters.getFaceId() == INVALID_FACE_ID) {
    NDN_THROW(ArgumentError("FaceId must be valid"));
  }
}

FaceUpdateCommand::FaceUpdateCommand()
  : ControlCommand("faces", "update")
{
  m_requestValidator
    .optional(CONTROL_PARAMETER_FACE_ID)
    .optional(CONTROL_PARAMETER_FACE_PERSISTENCY)
    .optional(CONTROL_PARAMETER_BASE_CONGESTION_MARKING_INTERVAL)
    .optional(CONTROL_PARAMETER_DEFAULT_CONGESTION_THRESHOLD)
    .optional(CONTROL_PARAMETER_MTU)
    .optional(CONTROL_PARAMETER_FLAGS)
    .optional(CONTROL_PARAMETER_MASK);
  m_responseValidator
    .required(CONTROL_PARAMETER_FACE_ID)
    .required(CONTROL_PARAMETER_FACE_PERSISTENCY)
    .optional(CONTROL_PARAMETER_BASE_CONGESTION_MARKING_INTERVAL)
    .optional(CONTROL_PARAMETER_DEFAULT_CONGESTION_THRESHOLD)
    .optional(CONTROL_PARAMETER_MTU)
    .required(CONTROL_PARAMETER_FLAGS);
}

void
FaceUpdateCommand::applyDefaultsToRequest(ControlParameters& parameters) const
{
  if (!parameters.hasFaceId()) {
    parameters.setFaceId(0);
  }
}

void
FaceUpdateCommand::validateResponse(const ControlParameters& parameters) const
{
  this->ControlCommand::validateResponse(parameters);

  if (parameters.getFaceId() == INVALID_FACE_ID) {
    NDN_THROW(ArgumentError("FaceId must be valid"));
  }
}

FaceDestroyCommand::FaceDestroyCommand()
  : ControlCommand("faces", "destroy")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_FACE_ID);
  m_responseValidator = m_requestValidator;
}

void
FaceDestroyCommand::validateRequest(const ControlParameters& parameters) const
{
  this->ControlCommand::validateRequest(parameters);

  if (parameters.getFaceId() == INVALID_FACE_ID) {
    NDN_THROW(ArgumentError("FaceId must be valid"));
  }
}

void
FaceDestroyCommand::validateResponse(const ControlParameters& parameters) const
{
  this->validateRequest(parameters);
}

FibAddNextHopCommand::FibAddNextHopCommand()
  : ControlCommand("fib", "add-nexthop")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_NAME)
    .optional(CONTROL_PARAMETER_FACE_ID)
    .optional(CONTROL_PARAMETER_COST);
  m_responseValidator
    .required(CONTROL_PARAMETER_NAME)
    .required(CONTROL_PARAMETER_FACE_ID)
    .required(CONTROL_PARAMETER_COST);
}

void
FibAddNextHopCommand::applyDefaultsToRequest(ControlParameters& parameters) const
{
  if (!parameters.hasFaceId()) {
    parameters.setFaceId(0);
  }
  if (!parameters.hasCost()) {
    parameters.setCost(0);
  }
}

void
FibAddNextHopCommand::validateResponse(const ControlParameters& parameters) const
{
  this->ControlCommand::validateResponse(parameters);

  if (parameters.getFaceId() == INVALID_FACE_ID) {
    NDN_THROW(ArgumentError("FaceId must be valid"));
  }
}

FibRemoveNextHopCommand::FibRemoveNextHopCommand()
  : ControlCommand("fib", "remove-nexthop")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_NAME)
    .optional(CONTROL_PARAMETER_FACE_ID);
  m_responseValidator
    .required(CONTROL_PARAMETER_NAME)
    .required(CONTROL_PARAMETER_FACE_ID);
}

void
FibRemoveNextHopCommand::applyDefaultsToRequest(ControlParameters& parameters) const
{
  if (!parameters.hasFaceId()) {
    parameters.setFaceId(0);
  }
}

void
FibRemoveNextHopCommand::validateResponse(const ControlParameters& parameters) const
{
  this->ControlCommand::validateResponse(parameters);

  if (parameters.getFaceId() == INVALID_FACE_ID) {
    NDN_THROW(ArgumentError("FaceId must be valid"));
  }
}

CsConfigCommand::CsConfigCommand()
  : ControlCommand("cs", "config")
{
  m_requestValidator
    .optional(CONTROL_PARAMETER_CAPACITY)
    .optional(CONTROL_PARAMETER_FLAGS)
    .optional(CONTROL_PARAMETER_MASK);
  m_responseValidator
    .required(CONTROL_PARAMETER_CAPACITY)
    .required(CONTROL_PARAMETER_FLAGS);
}

CsEraseCommand::CsEraseCommand()
  : ControlCommand("cs", "erase")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_NAME)
    .optional(CONTROL_PARAMETER_COUNT);
  m_responseValidator
    .required(CONTROL_PARAMETER_NAME)
    .optional(CONTROL_PARAMETER_CAPACITY)
    .required(CONTROL_PARAMETER_COUNT);
}

void
CsEraseCommand::validateRequest(const ControlParameters& parameters) const
{
  this->ControlCommand::validateRequest(parameters);

  if (parameters.hasCount() && parameters.getCount() == 0) {
    NDN_THROW(ArgumentError("Count must be positive"));
  }
}

void
CsEraseCommand::validateResponse(const ControlParameters& parameters) const
{
  this->ControlCommand::validateResponse(parameters);

  if (parameters.hasCapacity() && parameters.getCapacity() == 0) {
    NDN_THROW(ArgumentError("Capacity must be positive"));
  }
}

StrategyChoiceSetCommand::StrategyChoiceSetCommand()
  : ControlCommand("strategy-choice", "set")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_NAME)
    .required(CONTROL_PARAMETER_STRATEGY);
  m_responseValidator = m_requestValidator;
}

StrategyChoiceUnsetCommand::StrategyChoiceUnsetCommand()
  : ControlCommand("strategy-choice", "unset")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_NAME);
  m_responseValidator = m_requestValidator;
}

void
StrategyChoiceUnsetCommand::validateRequest(const ControlParameters& parameters) const
{
  this->ControlCommand::validateRequest(parameters);

  if (parameters.getName().empty()) {
    NDN_THROW(ArgumentError("Name must not be ndn:/"));
  }
}

void
StrategyChoiceUnsetCommand::validateResponse(const ControlParameters& parameters) const
{
  this->validateRequest(parameters);
}

RibRegisterCommand::RibRegisterCommand()
  : ControlCommand("rib", "register")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_NAME)
    .optional(CONTROL_PARAMETER_FACE_ID)
    .optional(CONTROL_PARAMETER_ORIGIN)
    .optional(CONTROL_PARAMETER_COST)
    .optional(CONTROL_PARAMETER_FLAGS)
    .optional(CONTROL_PARAMETER_EXPIRATION_PERIOD);
  m_responseValidator
    .required(CONTROL_PARAMETER_NAME)
    .required(CONTROL_PARAMETER_FACE_ID)
    .required(CONTROL_PARAMETER_ORIGIN)
    .required(CONTROL_PARAMETER_COST)
    .required(CONTROL_PARAMETER_FLAGS)
    .optional(CONTROL_PARAMETER_EXPIRATION_PERIOD);
}

void
RibRegisterCommand::applyDefaultsToRequest(ControlParameters& parameters) const
{
  if (!parameters.hasFaceId()) {
    parameters.setFaceId(0);
  }
  if (!parameters.hasOrigin()) {
    parameters.setOrigin(ROUTE_ORIGIN_APP);
  }
  if (!parameters.hasCost()) {
    parameters.setCost(0);
  }
  if (!parameters.hasFlags()) {
    parameters.setFlags(ROUTE_FLAG_CHILD_INHERIT);
  }
}

void
RibRegisterCommand::validateResponse(const ControlParameters& parameters) const
{
  this->ControlCommand::validateResponse(parameters);

  if (parameters.getFaceId() == INVALID_FACE_ID) {
    NDN_THROW(ArgumentError("FaceId must be valid"));
  }
}

RibUnregisterCommand::RibUnregisterCommand()
  : ControlCommand("rib", "unregister")
{
  m_requestValidator
    .required(CONTROL_PARAMETER_NAME)
    .optional(CONTROL_PARAMETER_FACE_ID)
    .optional(CONTROL_PARAMETER_ORIGIN);
  m_responseValidator
    .required(CONTROL_PARAMETER_NAME)
    .required(CONTROL_PARAMETER_FACE_ID)
    .required(CONTROL_PARAMETER_ORIGIN);
}

void
RibUnregisterCommand::applyDefaultsToRequest(ControlParameters& parameters) const
{
  if (!parameters.hasFaceId()) {
    parameters.setFaceId(0);
  }
  if (!parameters.hasOrigin()) {
    parameters.setOrigin(ROUTE_ORIGIN_APP);
  }
}

void
RibUnregisterCommand::validateResponse(const ControlParameters& parameters) const
{
  this->ControlCommand::validateResponse(parameters);

  if (parameters.getFaceId() == INVALID_FACE_ID) {
    NDN_THROW(ArgumentError("FaceId must be valid"));
  }
}

} // namespace nfd
} // namespace ndn
