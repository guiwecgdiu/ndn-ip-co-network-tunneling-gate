// gateway-app.cpp
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

#include <random>
#include <ctime> 

#include "gatewayApp.hpp"
#include "ns3/network-module.h"
#include "ns3/ipv4.h"

#include "theader.h"
#include "tnumheader.h"
#include "tipheader.h"


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

NS_LOG_COMPONENT_DEFINE ("GatewayApp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (GatewayApp);

// register NS-3 type
TypeId
GatewayApp::GetTypeId ()
{
  static TypeId tid = TypeId ("GatewayApp").SetParent<ndn::App> ().AddConstructor<GatewayApp> ();

  /*.AddAttribute ("GttRecords",
                   "The initiated GttRecords",
                   ObjectVectorValue (),
                   ObjectVectorAccessor (&GatewayApp::m_gtt),
                   ObjectVectorChecker ());*/

  //.AddAttribute("GttTable","Config a gtt table", StringValue(""),MakePointerAccessor(&GatewayApp::m_gtt),MakePointerChecker<GttTable>(),,"");
  return tid;
}

// Processing upon start of the application
void
GatewayApp::StartApplication ()
{
  //ip insert
  m_port1 = 7777;
  m_port2 = 9999;
  //ip insert

  
  

  //tunnel port
  m_tunnelPort = 7776;

  //gtt insert
  m_gtt = GttTable ();
  m_gtt.AddRoute (ndn::Name ("/domain1/src2"), Ipv4Address ("10.1.1.1"));
  m_gtt.AddRoute (ndn::Name ("/domain1/src1"), Ipv4Address ("10.1.1.1"));
  m_gtt.AddRoute (ndn::Name ("/domain1/iGate1"), Ipv4Address ("10.1.1.1"));
  m_gtt.AddRoute (ndn::Name ("/domain1/iGate2"), Ipv4Address ("10.1.5.2"));
  m_gtt.AddRoute (ndn::Name ("/domain2/dst1"), Ipv4Address ("10.1.5.2"));
  m_gtt.AddRoute (ndn::Name ("/domain2/dst2"), Ipv4Address ("10.1.5.2"));
  m_gtt.AddRoute (ndn::Name ("/domain2/dst3"), Ipv4Address ("10.1.5.2"));

  // initialize ndn::App
  ndn::App::StartApplication ();

  // Add entry to FIB for `/prefix/sub`
  ndn::FibHelper::AddRoute (GetNode (), "/tunnel", m_face, 0);

  //Simulator::Schedule(Seconds(5.0), &GatewayApp::SendInterest, this);
  //Schedule send of first interest
  Simulator::Schedule (Seconds (1.0), &GatewayApp::SendElection, this);

  //ip

  //Receive sockets
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_recv_socket1 = Socket::CreateSocket (Application::GetNode (), tid);
  m_recv_socket2 = Socket::CreateSocket (GetNode (), tid);
  m_recv_socketTunnel = Socket::CreateSocket (GetNode (), tid);
  

  ns3::Ptr<Node> i = GetNode ();
  int id = i->GetId ();
  NS_LOG_INFO (LIGHT_YELLOW_CODE << "GetNode " << id << "END_CODE");

  SetupReceiveSocket (m_recv_socket1, m_port1);
  SetupReceiveSocket (m_recv_socket2, m_port2);
  SetupReceiveSocket (m_recv_socketTunnel, m_tunnelPort);

  m_recv_socket1->SetRecvCallback (MakeCallback (&GatewayApp::HandleReadOne, this));
  m_recv_socket2->SetRecvCallback (MakeCallback (&GatewayApp::HandleReadTwo, this));
  m_recv_socketTunnel->SetRecvCallback (MakeCallback (&GatewayApp::HandleReadTunnelPort, this));


  //Send SocketSimulator::Schedule (Seconds (3), &SimpleUdpApplication::SendPacket, udp0, packet1,dest_ip_ip5 , 7777)
  m_send_socket = Socket::CreateSocket (GetNode (), tid);

  std::cout << "GatewayApp.start app." << std::endl;
  NS_LOG_INFO (TEAL_CODE << "Start App" << END_CODE);
  Ptr<Packet> packet1 = Create<Packet> (8000);
  Ipv4Address dest_ip_ip5 ("10.1.5.2");
  
  //Simulator::Schedule (Seconds (1), &GatewayApp::SendPacket, this, packet1,dest_ip_ip5 , 7777);
}

// Processing when application is stopped
void
GatewayApp::StopApplication ()
{
  // cleanup ndn::App
  ndn::App::StopApplication ();
}

void
GatewayApp::SendInterest ()
{
  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////

  // Create and configure ndn::Interest
  auto interest = std::make_shared<ndn::Interest> ("/domain2/dst3");
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  interest->setNonce (rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest->setInterestLifetime (ndn::time::seconds (1));

  NS_LOG_DEBUG ("Sending Interest packet for " << *interest);

  // Call trace (for logging purposes)
  m_transmittedInterests (interest, this, m_face);

  m_appLink->onReceiveInterest (*interest);
}

void
GatewayApp::ReformAndSendInterest (std::shared_ptr<ndn::Interest> interest)
{
  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////

  // Create and configure ndn::Interest
  //auto interest = std::make_shared<ndn::Interest>("/domain1/src1");

  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  interest->setNonce (rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest->setInterestLifetime (ndn::time::seconds (20));

  NS_LOG_INFO (RED_CODE << "Regenerate and Send Packet " << *interest << END_CODE);

  // Call trace (for logging purposes)
  m_transmittedInterests (interest, this, m_face);

  m_appLink->onReceiveInterest (*interest);
}

//
void
GatewayApp::SendElection ()
{
  
  /////////////////////////////////////
  // Sending one election Interest packet out //
  /////////////////////////////////////

  // Create and configure ndn::Interest
  std::shared_ptr<ndn::Interest> interest =
  std::make_shared<ndn::Interest> ("/tunnel/tunnelRegisty/");
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  interest->setNonce (rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest->setInterestLifetime (ndn::time::seconds (10000));

  NS_LOG_INFO (CYAN_CODE << "Sending Electron packet for " << *interest << " AT NODE "
                         << this->GetNode ()->GetId () << END_CODE);

  this->m_gtt.printTheMap ();
  // Call trace (for logging purposes)
  m_transmittedInterests (interest, this, m_face);

  m_appLink->onReceiveInterest (*interest);
}

// Callback that will be called when Interest arrives
void
GatewayApp::OnInterest (std::shared_ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest (interest);

  ndn::Block block = interest->wireEncode ();
  ndn::BlockHeader blockheader (block);
  Ptr<ns3::Packet> packet1 = Create<ns3::Packet> (block.size ());
  packet1->AddHeader (blockheader);

  NS_LOG_INFO (CYAN_CODE << "The Gateway program receive interest " << interest->toUri ()
                         << END_CODE);
  std::string interets_short = interest->toUri ();
  ndn::Block b = interest->wireEncode ();
  Ptr<Packet> packet = Create<Packet> ((uint8_t *) interets_short.c_str (), 8000);

  //read packet buffer/payload
  uint8_t *buffer = new uint8_t[packet->GetSize ()];
  packet->CopyData (buffer, packet->GetSize ());
  std::string s = std::string ((char *) buffer);

  //
  //read pkt pylod
  NS_LOG_INFO (BLUE_CODE << "Sending packet for " << s << END_CODE);

  //gtt mapping
  int size = interest->getName ().size ();
  ndn::Name name = interest->getName ().getSubName (0, size - 1);
  NS_LOG_INFO (RED_CODE << "gtt mapping input: " << name << END_CODE);
  ns3::Ipv4Address ip_str = m_gtt.mapToGateIP (name);
  NS_LOG_INFO (RED_CODE << "gtt mapping output: " << ip_str << END_CODE);


  //add ipaddress to header
  Ptr<Node> node = this->GetNode(); // Get pointer to ith node in container
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  uint32_t address= addr.Get();


  //add into packet
  ns3::TIPHeader TIPHeader;
  TIPHeader.SetData(address);
  packet1->AddHeader(TIPHeader);
  
  //if(find tunnel true) return ip_str and port number
  Ipv4Address dest_ip_ip5 (ip_str);
  Simulator::ScheduleNow (&GatewayApp::SendPacket, this, packet1, dest_ip_ip5, 7777);
  //test
  
  //this->BuildTunnel(dest_ip_ip5,newPort);
  //arno aug 9
  //test

//******** send general tunnel
  
  
  //else find false build then process

  /*
int newPort;;
    if (!m_available_port.empty())
    {
        newPort=m_available_port.back();
    }
    m_available_port.pop_back();

  if(1!=1)
  { 
    int newPort;;
    if (!m_available_port.empty())
    {
        newPort=m_available_port.back();
    }
    m_available_port.pop_back();
    this->BuildTunnel(dest_ip_ip5,newPort);
  }
  */
 //arno aug 9

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
GatewayApp::OnData (std::shared_ptr<const ndn::Data> data)
{
  NS_LOG_INFO (CYAN_CODE << "Receiving Data packet at handle two IN " << GetNode ()->GetId ()
                         << " WITH DATA " << data->getName () << END_CODE);

  std::cout << "DATA received for name " << data->getName ().getSubName(0,data->getName().size()-1) << std::endl;

  //data chunck need modify,gtt doesn't work
  //gtt mapping
  /*
  int size = data->getName().size();
  ndn::Name name =data->getName().getSubName(0,size-1);
  NS_LOG_INFO(RED_CODE<<name<<END_CODE);
  std::string ip_str = m_gtt.mapToGateIP(name);
   NS_LOG_INFO(RED_CODE<<ip_str<<END_CODE);
*/


  //Ipv4Address dest_ip_ip5 ("10.1.1.1");
  ndn::Name name = data->getName ().getSubName (0, data->getName().size()- 1);
  Ipv4Address dest_ip_ip5 = m_dtt.mapToGateIP(name);
  ndn::Block block = data->wireEncode ();
  ndn::BlockHeader blockheader (block);
  Ptr<ns3::Packet> packet1 = Create<ns3::Packet> (block.size ());
  packet1->AddHeader (blockheader);
  Simulator::ScheduleNow (&GatewayApp::SendPacket, this, packet1, dest_ip_ip5, 9999);
}

//GTT
void
GatewayApp::Gtt_addRoute (ndn::Name prefix, ns3::Ipv4Address ipv4address)
{
  m_gtt.AddRoute (prefix, ipv4address);
  m_gtt.printTheMap ();
}

//ip
void
GatewayApp::SetupReceiveSocket (Ptr<Socket> socket, uint16_t port)
{
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);
  if (socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
}

void
GatewayApp::HandleReadOne (Ptr<Socket> socket)
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
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      NS_LOG_INFO (TEAL_CODE << "HandleReadOne : Received a Packet of size: " << packet->GetSize ()
                             << " at time " << Now ().GetSeconds () << END_CODE);
      Ptr<ns3::Packet> recv_pkt = packet->Copy ();
    
      //fecth consumer ip
      TIPHeader tipHeader;
      recv_pkt->RemoveHeader (tipHeader);
      uint32_t ip = tipHeader.GetData ();
      Ipv4Address ipv4(ip);
      NS_LOG_INFO (GREEN_CODE<<"IP: " << ipv4 <<END_CODE);
      


      ndn::BlockHeader blockheader;
      recv_pkt->RemoveHeader (blockheader);
      ndn::Block recv_block = blockheader.getBlock ();
      auto interest = ndn::make_shared<ndn::Interest> (recv_block);
      //uint8_t *buffer = new uint8_t[packet->GetSize ()];
      //packet->CopyData(buffer, packet->GetSize ());
      std::string s = interest->toUri ();
      NS_LOG_INFO ("Content: " << s);
      m_dtt.AddRoute(interest->getName().getSubName(0,interest->getName().size()-1),ipv4);
      m_dtt.printDTTMap();
      ReformAndSendInterest (interest);
    }
}



void
GatewayApp::HandleReadTwo (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  int id = GetNode ()->GetId ();

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      NS_LOG_INFO (PURPLE_CODE << "HandleReadTwo : Received a Packet of size: "
                               << packet->GetSize () << " at time " << Now ().GetSeconds ()
                               << END_CODE);
      NS_LOG_INFO (CYAN_CODE << "Receiving Data packet at handle two IN " << id << " WITH DATA "
                             << END_CODE);
      Ptr<ns3::Packet> recv_pkt = packet->Copy ();
      ndn::BlockHeader blockheader;
      recv_pkt->RemoveHeader (blockheader);
      ndn::Block recv_block = blockheader.getBlock ();
      auto data = ndn::make_shared<ndn::Data> (recv_block);
      std::string s = data->getName ().toUri ();
      NS_LOG_INFO ("Data Content: " << s);
      ReformAndSendData (data);
    }
}


void
GatewayApp::BeforeTunnelBuild(Ptr<Socket> socket)
{
  NS_LOG_INFO(RED_CODE<<this->GetNode()->GetId()<<" beforeTunnelBuild "<<END_CODE);
}


void
GatewayApp::BuildTunnel (Ipv4Address destination, uint16_t port)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_Tunnel_echo_socket = Socket::CreateSocket (GetNode (), tid);
  SetupReceiveSocket (m_Tunnel_echo_socket, port);
    m_Tunnel_echo_socket -> SetRecvCallback (MakeCallback (&GatewayApp::BeforeTunnelBuild, this));

  
  //Ptr<Packet> packet = Create<Packet> ((uint8_t *) .c_str (), 8000);
  Ptr<ns3::Packet> p = Create<ns3::Packet> (200);
  ns3::THeader tHeader;
  uint32_t tempport = port;
  tHeader.SetData (tempport);
  // copy the header into the packet
  p->AddHeader (tHeader);
  NS_LOG_INFO(PURPLE_CODE<<this->GetNode()->GetId()<<" 's listen port: "<<tempport<<" : " <<END_CODE);


  //get the ip address
  Ptr<Node> node = this->GetNode(); // Get pointer to ith node in container
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  uint32_t address= addr.Get();
  NS_LOG_INFO(PURPLE_CODE<<this->GetNode()->GetId()<<" 's ip: "<<address<<" : " <<addr<<END_CODE);


  //add into packet
  ns3::TIPHeader TIPHeader;
  TIPHeader.SetData(address);
  p->AddHeader(TIPHeader);
  

  //add the tracing number
  ns3::TNUMHeader TNUMHeader;
  //TNUMHeader.SetData();

  static std::random_device rd; // random device engine, usually based on /dev/random on UNIX-like systems
  // initialize Mersennes' twister using rd to generate the seed
  static std::mt19937 rng{rd()}; 
  static std::uniform_int_distribution<int> uid(1,6); // random dice
  int16_t sequence=uid(rng); // use rng as a generator

  TNUMHeader.SetData(sequence);
  p->AddHeader(TNUMHeader);
  
  Simulator::ScheduleNow (&GatewayApp::SendPacket, this, p,destination, this->m_tunnelPort);
  
}

void
GatewayApp::HandleReadTunnelPort (Ptr<Socket> socket)
{

  NS_LOG_INFO(PURPLE_CODE<<"receive a tunnel request");

  int id = GetNode ()->GetId ();

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  uint16_t socketPort_req;  //the requester socket
  uint32_t ip_req;
  int32_t sequence;

  while ((packet = socket->RecvFrom (from)))
    {
      Ptr<ns3::Packet> recv_pkt = packet->Copy ();
      TIPHeader tipHeader;
      THeader tHeader;
      TNUMHeader tnumHeader;
  // copy the header from the packet
  recv_pkt->RemoveHeader (tnumHeader);
      sequence = tnumHeader.GetData();
      NS_LOG_INFO(PURPLE_CODE<<this->GetNode()->GetId()<<" get sequence: "<<sequence<<" : " <<END_CODE);
      recv_pkt->RemoveHeader (tipHeader);
      uint32_t ip = tipHeader.GetData ();
      NS_LOG_INFO(PURPLE_CODE<<this->GetNode()->GetId()<<" get ip: "<<ip_req<<" : " <<END_CODE);
      recv_pkt->RemoveHeader (tHeader);
      socketPort_req = tHeader.GetData();
      NS_LOG_INFO(PURPLE_CODE<<this->GetNode()->GetId()<<" get socket: "<<socketPort_req<<" : " <<END_CODE);
      ///remove order matters!!!!!!!!!!! sequnece -> ip -> socket.

/////////////////////////////////////////////////////////////
  //echo back with info

  //Ptr<Packet> packet = Create<Packet> ((uint8_t *) .c_str (), 8000);
  Ptr<ns3::Packet> back = Create<ns3::Packet> (200);
  ns3::THeader tH;
  uint32_t tempport = 2222; //wait for modify arno
  tH.SetData (tempport);
  // copy the header into the packet
  back->AddHeader (tH);
  NS_LOG_INFO(GREEN_CODE<<this->GetNode()->GetId()<<" 's listen port: "<<tempport<<" : " <<END_CODE);


  //get the ip address
  Ptr<Node> node = this->GetNode(); // Get pointer to ith node in container
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  uint32_t address= addr.Get();
  NS_LOG_INFO(GREEN_CODE<<this->GetNode()->GetId()<<" 's ip: "<<address<<" : " <<addr<<END_CODE);


  //add into packet
  ns3::TIPHeader TIPHeader;
  TIPHeader.SetData(address);
  back->AddHeader(TIPHeader);
  

  //add the tracing number
  ns3::TNUMHeader TNUMHeader;
  //TNUMHeader.SetData();
  TNUMHeader.SetData(sequence);
  back->AddHeader(TNUMHeader);

  Ipv4Address requester_ip(ip_req);
  Simulator::ScheduleNow (&GatewayApp::SendPacket, this, back,requester_ip, socketPort_req);
  
  }
}

void
GatewayApp::ReformAndSendData (std::shared_ptr<ndn::Data> data)
{

  ndn::StackHelper::getKeyChain ().sign (*data);

  NS_LOG_INFO (PURPLE_CODE << "Sending Data packet for " << data->getName () << END_CODE);
  // Call trace (for logging purposes)
  m_transmittedDatas (data, this, m_face);

  m_appLink->onReceiveData (*data);

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

void
GatewayApp::SendPacket (Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
{
  NS_LOG_FUNCTION (this << packet << destination << port);
  m_send_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (destination), port));
  m_send_socket->Send (packet);
}

} // namespace ns3
