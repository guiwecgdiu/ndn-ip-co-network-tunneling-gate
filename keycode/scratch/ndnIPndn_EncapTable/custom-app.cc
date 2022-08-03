// custom-app.cpp
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

#include "custom-app.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "ns3/random-variable-stream.h"
#include "ndn-cxx/encoding/block.hpp"
#include "ndn-cxx/encoding/buffer-stream.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ns3/ndnSIM/model/ndn-block-header.hpp"

NS_LOG_COMPONENT_DEFINE("CustomApp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(CustomApp);

// register NS-3 type
TypeId
CustomApp::GetTypeId()
{
  static TypeId tid = TypeId("CustomApp").SetParent<ndn::App>().AddConstructor<CustomApp>();
  return tid;
}



// Processing upon start of the application
void
CustomApp::StartApplication()
{
  //ip insert
  m_port1 = 7777;
  m_port2 = 9999;
  //ip insert


  // initialize ndn::App
  ndn::App::StartApplication();

  // Add entry to FIB for `/prefix/sub`
  ndn::FibHelper::AddRoute(GetNode(), "/tunnel", m_face, 0);
  
  //Simulator::Schedule(Seconds(5.0), &CustomApp::SendInterest, this);
  //Schedule send of first interest
  Simulator::Schedule(Seconds(1.0), &CustomApp::SendElection, this);

//ip

//Receive sockets
TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
m_recv_socket1 = Socket::CreateSocket(Application::GetNode(), tid);
m_recv_socket2 = Socket::CreateSocket(GetNode(), tid);
ns3::Ptr<Node> i =GetNode();
int id= i->GetId();
NS_LOG_INFO(LIGHT_YELLOW_CODE <<"GetNode "<< id << "END_CODE");

SetupReceiveSocket(m_recv_socket1, m_port1);
SetupReceiveSocket(m_recv_socket2, m_port2);

m_recv_socket1->SetRecvCallback(MakeCallback(&CustomApp::HandleReadOne, this));
m_recv_socket2->SetRecvCallback(MakeCallback(&CustomApp::HandleReadTwo, this));

//Send SocketSimulator::Schedule (Seconds (3), &SimpleUdpApplication::SendPacket, udp0, packet1,dest_ip_ip5 , 7777)
m_send_socket = Socket::CreateSocket(GetNode(), tid);

std::cout << "customApp.start app." << std::endl;
NS_LOG_INFO(TEAL_CODE <<"Start App"<<END_CODE);
Ptr <Packet> packet1 = Create <Packet> (8000);
Ipv4Address dest_ip_ip5 ("10.1.5.2");
//Simulator::Schedule (Seconds (1), &CustomApp::SendPacket, this, packet1,dest_ip_ip5 , 7777);

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
  auto interest = std::make_shared<ndn::Interest>("/domain2/dst3");
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));

  NS_LOG_DEBUG("Sending Interest packet for " << *interest);

  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  m_appLink->onReceiveInterest(*interest);
}

void
CustomApp::ReformAndSendInterest(std::shared_ptr<ndn::Interest> interest)
{
  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////

  // Create and configure ndn::Interest
  //auto interest = std::make_shared<ndn::Interest>("/domain1/src1");

  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(20));

  NS_LOG_INFO(RED_CODE<<"Regenerate and Send Packet " << *interest<<END_CODE);

  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  m_appLink->onReceiveInterest(*interest);
}



//
void
CustomApp::SendElection()
{
 /////////////////////////////////////
  // Sending one election Interest packet out //
  /////////////////////////////////////

  // Create and configure ndn::Interest
  std::shared_ptr<ndn::Interest> interest = std::make_shared<ndn::Interest>("/tunnel/tunnelRegisty/");
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(10000));
  

  NS_LOG_INFO(CYAN_CODE<<"Sending Electron packet for " << *interest<<" AT NODE "<<this->GetNode()->GetId()<<END_CODE);

  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  m_appLink->onReceiveInterest(*interest);

}

// Callback that will be called when Interest arrives
void
CustomApp::OnInterest(std::shared_ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest(interest);


  ndn::Block block = interest->wireEncode();
  ndn::BlockHeader blockheader(block);
  Ptr <ns3::Packet> packet1 = Create <ns3::Packet> (block.size());
  packet1->AddHeader(blockheader);


  NS_LOG_INFO(CYAN_CODE<<"The Gateway program receive interest "<<interest->toUri()<<END_CODE);
  std::string interets_short=interest->toUri();
  ndn::Block b=interest->wireEncode();
  Ptr<Packet> packet = Create<Packet> ((uint8_t*)interets_short.c_str(), 8000); 
  
  //read packet buffer/payload
  uint8_t *buffer = new uint8_t[packet->GetSize ()];
  packet->CopyData(buffer, packet->GetSize ());
  std::string s = std::string((char*)buffer);
  
  
  //read pkt pylod
   NS_LOG_INFO(BLUE_CODE<<"Sending packet for " << s<<END_CODE);
  Ipv4Address dest_ip_ip5 ("10.1.5.2");
  Simulator::ScheduleNow(&CustomApp::SendPacket, this, packet1,dest_ip_ip5 , 7777);
  // Note that Interests send out by the app will not be sent back to the app !


  /*
  auto data = std::make_shared<ndn::Data>(interest->getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  data->setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending Data packet for " << data->getName());
  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
  */


  
}

// Callback that will be called when Data arrives
void
CustomApp::OnData(std::shared_ptr<const ndn::Data> data)
{
  NS_LOG_INFO(CYAN_CODE<<"Receiving Data packet at handle two IN " <<GetNode()->GetId()<<" WITH DATA "<< data->getName()<<END_CODE);

  std::cout << "DATA received for name " << data->getName() << std::endl;
   
  Ipv4Address dest_ip_ip5 ("10.1.1.1");
  ndn::Block block=data->wireEncode();
  ndn::BlockHeader blockheader(block);
  Ptr <ns3::Packet> packet1 = Create <ns3::Packet> (block.size());
  packet1->AddHeader(blockheader);
  Simulator::ScheduleNow(&CustomApp::SendPacket, this, packet1,dest_ip_ip5 , 9999);
}


//ip
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
    /*
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
      NS_LOG_INFO(TEAL_CODE << "HandleReadOne : Received a Packet of size: " << packet->GetSize() << " at time " << Now().GetSeconds() << END_CODE);
      NS_LOG_INFO("Content: " << packet->ToString());
    }
    */
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
      NS_LOG_INFO(TEAL_CODE << "HandleReadOne : Received a Packet of size: " << packet->GetSize() << " at time " << Now().GetSeconds() << END_CODE);
      Ptr<ns3::Packet> recv_pkt=packet->Copy();
      ndn::BlockHeader blockheader;
      recv_pkt->RemoveHeader(blockheader);
      ndn::Block recv_block=blockheader.getBlock();
      auto interest = ndn::make_shared<ndn::Interest>(recv_block);
      //uint8_t *buffer = new uint8_t[packet->GetSize ()];
      //packet->CopyData(buffer, packet->GetSize ());
      std::string s = interest->toUri();
      NS_LOG_INFO("Content: " << s);
      ReformAndSendInterest(interest);
    }
  }


   void CustomApp::HandleReadTwo(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);
    int id=GetNode()->GetId();
    
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
      NS_LOG_INFO(PURPLE_CODE << "HandleReadTwo : Received a Packet of size: " << packet->GetSize() << " at time " << Now().GetSeconds() << END_CODE);
      NS_LOG_INFO(CYAN_CODE<<"Receiving Data packet at handle two IN " << id <<" WITH DATA "<<END_CODE);
      Ptr<ns3::Packet> recv_pkt=packet->Copy();
      ndn::BlockHeader blockheader;
      recv_pkt->RemoveHeader(blockheader);
      ndn::Block recv_block=blockheader.getBlock();
      auto data = ndn::make_shared<ndn::Data>(recv_block);
      std::string s = data->getName().toUri();
      NS_LOG_INFO("Data Content: " << s);
      ReformAndSendData(data);
    }
  }

  void
CustomApp::ReformAndSendData(std::shared_ptr<ndn::Data> data)
{
  
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_INFO(PURPLE_CODE<<"Sending Data packet for " << data->getName()<<END_CODE);
  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
  
  /*
  auto data = std::make_shared<ndn::Data>(interest->getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  data->setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending Data packet for " << data->getName());
  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
  */

}

  void CustomApp::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
  {
    NS_LOG_FUNCTION (this << packet << destination << port);
    m_send_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(destination), port));
    m_send_socket->Send(packet);
  }


} // namespace ns3
