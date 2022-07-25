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

#include "ndn-cxx/util/dummy-client-face.hpp"
#include "ndn-cxx/impl/lp-field-tag.hpp"
#include "ndn-cxx/lp/packet.hpp"
#include "ndn-cxx/lp/tags.hpp"
#include "ndn-cxx/mgmt/nfd/controller.hpp"
#include "ndn-cxx/mgmt/nfd/control-response.hpp"
#include "ndn-cxx/transport/transport.hpp"

#include <boost/asio/io_service.hpp>

namespace ndn {
namespace util {

class DummyClientFace::Transport final : public ndn::Transport
{
public:
  void
  receive(Block block) const
  {
    block.encode();
    if (m_receiveCallback) {
      m_receiveCallback(block);
    }
  }

  void
  send(const Block& block) final
  {
    onSendBlock(block);
  }

  void
  close() final
  {
  }

  void
  pause() final
  {
  }

  void
  resume() final
  {
  }

public:
  Signal<Transport, Block> onSendBlock;
};

struct DummyClientFace::BroadcastLink
{
  std::vector<DummyClientFace*> faces;
};

DummyClientFace::AlreadyLinkedError::AlreadyLinkedError()
  : Error("Face has already been linked to another face")
{
}

DummyClientFace::DummyClientFace(const Options& options)
  : Face(make_shared<DummyClientFace::Transport>())
  , m_internalKeyChain(make_unique<KeyChain>())
  , m_keyChain(*m_internalKeyChain)
{
  this->construct(options);
}

DummyClientFace::DummyClientFace(KeyChain& keyChain, const Options& options)
  : Face(make_shared<DummyClientFace::Transport>(), keyChain)
  , m_keyChain(keyChain)
{
  this->construct(options);
}

DummyClientFace::DummyClientFace(boost::asio::io_service& ioService, const Options& options)
  : Face(make_shared<DummyClientFace::Transport>(), ioService)
  , m_internalKeyChain(make_unique<KeyChain>())
  , m_keyChain(*m_internalKeyChain)
{
  this->construct(options);
}

DummyClientFace::DummyClientFace(boost::asio::io_service& ioService, KeyChain& keyChain, const Options& options)
  : Face(make_shared<DummyClientFace::Transport>(), ioService, keyChain)
  , m_keyChain(keyChain)
{
  this->construct(options);
}

DummyClientFace::~DummyClientFace()
{
  unlink();
}

void
DummyClientFace::construct(const Options& options)
{
  static_pointer_cast<Transport>(getTransport())->onSendBlock.connect([this] (Block packet) {
    packet.encode();
    lp::Packet lpPacket(packet);
    auto frag = lpPacket.get<lp::FragmentField>();
    Block block({frag.first, frag.second});

    if (block.type() == tlv::Interest) {
      auto interest = make_shared<Interest>(block);
      if (lpPacket.has<lp::NackField>()) {
        auto nack = make_shared<lp::Nack>(std::move(*interest));
        nack->setHeader(lpPacket.get<lp::NackField>());
        addTagFromField<lp::CongestionMarkTag, lp::CongestionMarkField>(*nack, lpPacket);
        onSendNack(*nack);
      }
      else {
        addTagFromField<lp::NextHopFaceIdTag, lp::NextHopFaceIdField>(*interest, lpPacket);
        addTagFromField<lp::CongestionMarkTag, lp::CongestionMarkField>(*interest, lpPacket);
        onSendInterest(*interest);
      }
    }
    else if (block.type() == tlv::Data) {
      auto data = make_shared<Data>(block);
      addTagFromField<lp::CachePolicyTag, lp::CachePolicyField>(*data, lpPacket);
      addTagFromField<lp::CongestionMarkTag, lp::CongestionMarkField>(*data, lpPacket);
      onSendData(*data);
    }
  });

  if (options.enablePacketLogging)
    this->enablePacketLogging();

  if (options.enableRegistrationReply)
    this->enableRegistrationReply(options.registrationReplyFaceId);

  m_processEventsOverride = options.processEventsOverride;

  enableBroadcastLink();
}

void
DummyClientFace::enableBroadcastLink()
{
  this->onSendInterest.connect([this] (const Interest& interest) {
      if (m_bcastLink != nullptr) {
        for (auto otherFace : m_bcastLink->faces) {
          if (otherFace != this) {
            otherFace->receive(interest);
          }
        }
      }
    });
  this->onSendData.connect([this] (const Data& data) {
      if (m_bcastLink != nullptr) {
        for (auto otherFace : m_bcastLink->faces) {
          if (otherFace != this) {
            otherFace->receive(data);
          }
        }
      }
    });
  this->onSendNack.connect([this] (const lp::Nack& nack) {
      if (m_bcastLink != nullptr) {
        for (auto otherFace : m_bcastLink->faces) {
          if (otherFace != this) {
            otherFace->receive(nack);
          }
        }
      }
    });
}

void
DummyClientFace::enablePacketLogging()
{
  onSendInterest.connect([this] (const Interest& interest) {
    this->sentInterests.push_back(interest);
  });
  onSendData.connect([this] (const Data& data) {
    this->sentData.push_back(data);
  });
  onSendNack.connect([this] (const lp::Nack& nack) {
    this->sentNacks.push_back(nack);
  });
}

void
DummyClientFace::enableRegistrationReply(uint64_t faceId)
{
  onSendInterest.connect([=] (const Interest& interest) {
    static const Name localhostRibPrefix("/localhost/nfd/rib");
    static const name::Component registerVerb("register");
    const auto& name = interest.getName();
    if (name.size() <= 4 || !localhostRibPrefix.isPrefixOf(name))
      return;

    nfd::ControlParameters params(name[4].blockFromValue());
    if (!params.hasFaceId()) {
      params.setFaceId(faceId);
    }
    if (!params.hasOrigin()) {
      params.setOrigin(nfd::ROUTE_ORIGIN_APP);
    }
    if (!params.hasCost() && name[3] == registerVerb) {
      params.setCost(0);
    }

    nfd::ControlResponse resp;
    resp.setCode(200);
    resp.setBody(params.wireEncode());

    shared_ptr<Data> data = make_shared<Data>(name);
    data->setContent(resp.wireEncode());
    m_keyChain.sign(*data, security::SigningInfo(security::SigningInfo::SIGNER_TYPE_SHA256));
    this->getIoService().post([this, data] { this->receive(*data); });
  });
}

void
DummyClientFace::receive(const Interest& interest)
{
  lp::Packet lpPacket(interest.wireEncode());

  addFieldFromTag<lp::IncomingFaceIdField, lp::IncomingFaceIdTag>(lpPacket, interest);
  addFieldFromTag<lp::NextHopFaceIdField, lp::NextHopFaceIdTag>(lpPacket, interest);
  addFieldFromTag<lp::CongestionMarkField, lp::CongestionMarkTag>(lpPacket, interest);

  static_pointer_cast<Transport>(getTransport())->receive(lpPacket.wireEncode());
}

void
DummyClientFace::receive(const Data& data)
{
  lp::Packet lpPacket(data.wireEncode());

  addFieldFromTag<lp::IncomingFaceIdField, lp::IncomingFaceIdTag>(lpPacket, data);
  addFieldFromTag<lp::CongestionMarkField, lp::CongestionMarkTag>(lpPacket, data);

  static_pointer_cast<Transport>(getTransport())->receive(lpPacket.wireEncode());
}

void
DummyClientFace::receive(const lp::Nack& nack)
{
  lp::Packet lpPacket;
  lpPacket.add<lp::NackField>(nack.getHeader());
  Block interest = nack.getInterest().wireEncode();
  lpPacket.add<lp::FragmentField>({interest.begin(), interest.end()});

  addFieldFromTag<lp::IncomingFaceIdField, lp::IncomingFaceIdTag>(lpPacket, nack);
  addFieldFromTag<lp::CongestionMarkField, lp::CongestionMarkTag>(lpPacket, nack);

  static_pointer_cast<Transport>(getTransport())->receive(lpPacket.wireEncode());
}

void
DummyClientFace::linkTo(DummyClientFace& other)
{
  if (m_bcastLink != nullptr && other.m_bcastLink != nullptr) {
    if (m_bcastLink != other.m_bcastLink) {
      // already on different links
      NDN_THROW(AlreadyLinkedError());
    }
  }
  else if (m_bcastLink == nullptr && other.m_bcastLink != nullptr) {
    m_bcastLink = other.m_bcastLink;
    m_bcastLink->faces.push_back(this);
  }
  else if (m_bcastLink != nullptr && other.m_bcastLink == nullptr) {
    other.m_bcastLink = m_bcastLink;
    m_bcastLink->faces.push_back(&other);
  }
  else {
    m_bcastLink = other.m_bcastLink = make_shared<BroadcastLink>();
    m_bcastLink->faces.push_back(this);
    m_bcastLink->faces.push_back(&other);
  }
}

void
DummyClientFace::unlink()
{
  if (m_bcastLink == nullptr) {
    return;
  }

  auto it = std::find(m_bcastLink->faces.begin(), m_bcastLink->faces.end(), this);
  BOOST_ASSERT(it != m_bcastLink->faces.end());
  m_bcastLink->faces.erase(it);

  if (m_bcastLink->faces.size() == 1) {
    m_bcastLink->faces[0]->m_bcastLink = nullptr;
    m_bcastLink->faces.clear();
  }
  m_bcastLink = nullptr;
}

void
DummyClientFace::doProcessEvents(time::milliseconds timeout, bool keepThread)
{
  if (m_processEventsOverride != nullptr) {
    m_processEventsOverride(timeout);
  }
  else {
    this->Face::doProcessEvents(timeout, keepThread);
  }
}

} // namespace util
} // namespace ndn
