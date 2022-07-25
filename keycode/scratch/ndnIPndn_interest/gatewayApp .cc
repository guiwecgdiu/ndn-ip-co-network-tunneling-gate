/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// custom-app.cpp

#include "gatewayApp.h"
#include "ns3/simulator.h"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "ns3/random-variable-stream.h"

#include "ns3/log.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#define PURPLE_CODE "\033[95m"
#define CYAN_CODE "\033[96m"
#define TEAL_CODE "\033[36m"
#define BLUE_CODE "\033[94m"
#define GREEN_CODE "\033[32m"
#define YELLOW_CODE "\033[33m"
#define LIGHT_YELLOW_CODE "\033[93m"
#define RED_CODE "\033[91m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

NS_LOG_COMPONENT_DEFINE("CustomApp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(CustomApp);

// register NS-3 type
TypeId
CustomApp::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ndn::GatewayApp").SetParent<ndn::App>().AddConstructor<CustomApp>();
  return tid;
}

void CustomApp::SetupReceiveSocket(Ptr<Socket> socket, uint16_t port)
  {
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
    if (socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
  }
 void CustomApp::HandleReadOne(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
      NS_LOG_INFO(TEAL_CODE << "HandleReadOne : Received a Packet of size: " << packet->GetSize() << " at time " << Now().GetSeconds() << END_CODE);
      NS_LOG_INFO("Content: " << packet->ToString());
    }
  }

  void CustomApp::HandleReadTwo(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
      NS_LOG_INFO(PURPLE_CODE << "HandleReadTwo : Received a Packet of size: " << packet->GetSize() << " at time " << Now().GetSeconds() << END_CODE);
      NS_LOG_INFO("Content: " << packet->ToString());
    }
  }

// Processing upon start of the application
void
CustomApp::StartApplication()
{ 
   m_port1 = 7777;
    m_port2 = 9999;
  // initialize ndn::App
  ndn::App::StartApplication();

  // Add entry to FIB for `/prefix/sub`
  ndn::FibHelper::AddRoute(GetNode(), "/prefix/sub", m_face, 0);

  // Schedule send of first interest
  Simulator::Schedule(Seconds(1.0), &CustomApp::SendInterest, this);

  //Receive sockets
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_recv_socket1 = Socket::CreateSocket(Application::GetNode(), tid);
    m_recv_socket2 = Socket::CreateSocket(GetNode(), tid);

    SetupReceiveSocket(m_recv_socket1, m_port1);
    SetupReceiveSocket(m_recv_socket2, m_port2);

    m_recv_socket1->SetRecvCallback(MakeCallback(&CustomApp::HandleReadOne, this));
    m_recv_socket2->SetRecvCallback(MakeCallback(&CustomApp::HandleReadTwo, this));

    //Send SocketSimulator::Schedule (Seconds (3), &SimpleUdpApplication::SendPacket, udp0, packet1,dest_ip_ip5 , 7777)
    m_send_socket = Socket::CreateSocket(GetNode(), tid);
    Ptr <Packet> packet1 = Create <Packet> (8000);
    Ipv4Address dest_ip_ip5 ("10.1.5.2");
    Simulator::Schedule (Seconds (7), &CustomApp::SendPacket, this, packet1,dest_ip_ip5 , 7777);

}

 void CustomApp::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
  {
    NS_LOG_FUNCTION (this << packet << destination << port);
    m_send_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(destination), port));
    m_send_socket->Send(packet);
  }

// Processing when application is stopped
void
CustomApp::StopApplication()
{
  // cleanup ndn::App
  ndn::App::StopApplication();
}

void
CustomApp::SendInterest()
{
  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////

  // Create and configure ndn::Interest
  auto interest = std::make_shared<ndn::Interest>("/prefix/sub");
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));

  NS_LOG_DEBUG("Sending Interest packet for " << *interest);

  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  m_appLink->onReceiveInterest(*interest);
}

// Callback that will be called when Interest arrives
void
CustomApp::OnInterest(std::shared_ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest(interest);

  NS_LOG_DEBUG("Received Interest packet for " << interest->getName());

  // Note that Interests send out by the app will not be sent back to the app !

  auto data = std::make_shared<ndn::Data>(interest->getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  data->setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending Data packet for " << data->getName());

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
}

// Callback that will be called when Data arrives
void
CustomApp::OnData(std::shared_ptr<const ndn::Data> data)
{
  NS_LOG_DEBUG("Receiving Data packet for " << data->getName());

  std::cout << "DATA received for name " << data->getName() << std::endl;
}

} // namespace ns3