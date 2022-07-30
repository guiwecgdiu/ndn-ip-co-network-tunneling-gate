// ndn-congestion-topo-plugin.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/csma-helper.h"
#include "ns3/ipv4-address.h"

#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "simpleUDPApp.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ndn-load-balancer/random-load-balancer-strategy.hpp"
#include "NFD/daemon/fw/gatewayTunnelStrategy.hpp"
#include "custom-app.hpp"



using namespace ns3;
using ns3::ndn::StrategyChoiceHelper;

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);  

  MobilityHelper mobilitys;
  Ptr<ListPositionAllocator> initialAlloc =
  CreateObject<ListPositionAllocator> ();
   initialAlloc->Add(Vector(75, -35, 0.));
   initialAlloc->Add(Vector(75, -65, 0.));
   initialAlloc->Add(Vector(120, -50., 0.));
   initialAlloc->Add(Vector(280, -50., 0.));
   initialAlloc->Add(Vector(330, -65., 0.));
   initialAlloc->Add(Vector(320, -25., 0.));
   initialAlloc->Add(Vector(350, -75., 0.));

  mobilitys.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilitys.SetPositionAllocator(initialAlloc);
  

  NodeContainer nodes0;
  nodes0.Create(7);
  mobilitys.Install(nodes0);
  PointToPointHelper p2ps;
  p2ps.Install(nodes0.Get(0), nodes0.Get(2));
  p2ps.Install(nodes0.Get(1), nodes0.Get(2));
  p2ps.Install(nodes0.Get(3), nodes0.Get(5));
  p2ps.Install(nodes0.Get(4), nodes0.Get(6));
  p2ps.Install(nodes0.Get(3), nodes0.Get(4));
  p2ps.Install(nodes0.Get(5), nodes0.Get(6));

  
  Ptr<Node> node1 =nodes0.Get(0);
  Ptr<Node> node2 = nodes0.Get(1);
  Ptr<Node> node3 = nodes0.Get(4);
  Ptr<Node> node4 = nodes0.Get(6);
  Ptr<Node> node5 = nodes0.Get(5);
	Ptr<Node> iGate1 = nodes0.Get(2);
	Ptr<Node> iGate2 = nodes0.Get(3);



  //IP backbone/links, includes 4 nodes.
  //assign the postion to nodes, aiming for a easy dislay
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(150, -50, 0)); // node0
  positionAlloc ->Add(Vector(170, -70, 0)); // node1
  positionAlloc ->Add(Vector(230, -50, 0)); // node2
  positionAlloc ->Add(Vector(250, -60, 0)); // node3
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
  
  //initiate 4 nodes and install the position information on them
  NodeContainer nodes;
  nodes.Create(4);
  mobility.Install(nodes);
  


  //Create three domain, central, border1, and border2
  NetDeviceContainer netDev0,netDev1,netDev2,netDev3,netDev4;
  PointToPointHelper p2p;
  netDev0=p2p.Install(iGate1,nodes.Get(0));
  netDev1=p2p.Install(nodes.Get(0), nodes.Get(1));
  netDev2=p2p.Install(nodes.Get(1), nodes.Get(2));
  netDev3=p2p.Install(nodes.Get(2), nodes.Get(3));
  netDev4=p2p.Install(nodes.Get(3), iGate2);
  
 
 

  //install the network stacks.to emplty nodes
  InternetStackHelper stack;
  stack.Install(nodes);
  stack.Install(iGate1);
  stack.Install(iGate2);
  


  //assign the Ip address for network devices, central for 10.1.1.0
  //border 1 for 10.1.2.0
  //border 2 for 10.1.3.0
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces;
  address.Assign (netDev0);
  address.SetBase("10.1.2.0","255.255.255.0");
  Ipv4InterfaceContainer ifaces1;
  address.Assign (netDev1);
  address.SetBase("10.1.3.0","255.255.255.0");
  address.Assign (netDev2);
   address.SetBase("10.1.4.0","255.255.255.0");
  address.Assign (netDev3);
  address.SetBase("10.1.5.0","255.255.255.0");
  address.Assign (netDev4);

  //Fill the Routing tables
  

  ////Create 4 Two UDP applications , 1 app for each nodes
  Ptr <SimpleUdpApplication> udp0 = CreateObject <SimpleUdpApplication> ();
  Ptr <SimpleUdpApplication> udp1 = CreateObject <SimpleUdpApplication> ();
  Ptr <SimpleUdpApplication> udp2 = CreateObject <SimpleUdpApplication> ();
  Ptr <SimpleUdpApplication> udp3 = CreateObject <SimpleUdpApplication> ();
  Ptr <SimpleUdpApplication> udp4 = CreateObject <SimpleUdpApplication> ();
  Ptr <SimpleUdpApplication> udp5 = CreateObject <SimpleUdpApplication> ();
  


   //Set the start & stop times, start from 0s and end at 10s
  udp0->SetStartTime (Seconds(0));
  udp0->SetStopTime (Seconds (10));
  udp1->SetStartTime (Seconds(0));
  udp1->SetStopTime (Seconds (10));
  udp2->SetStartTime (Seconds(0));
  udp2->SetStopTime (Seconds (10));
  udp3->SetStartTime (Seconds(0));
  udp3->SetStopTime (Seconds (10));
  udp4->SetStartTime (Seconds(0));
  udp4->SetStopTime (Seconds (10));
  udp5->SetStartTime (Seconds(0));
  udp5->SetStopTime (Seconds (10));
  


  //install app 0,1,2,3 in ip node 0,1,2,3
  nodes.Get(0)->AddApplication (udp1);
  nodes.Get(1)->AddApplication (udp2);
  nodes.Get(2)->AddApplication (udp3);
  nodes.Get(3)->AddApplication (udp4);
  //iGate1->AddApplication(udp0);
  //iGate2->AddApplication(udp5);
  

  //TRead from the topologies and explict the ip address for each nodes, in a same index.
  Ipv4Address dest_ip_ip0 ("10.1.1.1");
  Ipv4Address dest_ip_ip1 ("10.1.2.1");
  Ipv4Address dest_ip_ip2 ("10.1.3.1");
  Ipv4Address dest_ip_ip3 ("10.1.4.1");
  Ipv4Address dest_ip_ip4 ("10.1.5.1");
  Ipv4Address dest_ip_ip5 ("10.1.5.2");

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();


  //ndn part
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(node1);
	ndnHelper.Install(node2);
	ndnHelper.Install(node3);
	ndnHelper.Install(node4);
  ndnHelper.Install(node5);
	ndnHelper.Install(iGate1);
	ndnHelper.Install(iGate2);
  
  NodeContainer ndnNodes;
  ndnNodes.Add(node1);
  ndnNodes.Add(node2);
  ndnNodes.Add(node3);
  ndnNodes.Add(node4);
  ndnNodes.Add(node5);
  ndnNodes.Add(iGate1);
  ndnNodes.Add(iGate2);

  
  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::Install(ndnNodes,"/","/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install<nfd::fw::GatewayTunnelStrategy>(iGate1,"/");
   ndn::StrategyChoiceHelper::Install<nfd::fw::GatewayTunnelStrategy>(iGate2,"/");




  //Initiate the producer app
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  ndn::AppHelper gatewayHelper("CustomApp");
  gatewayHelper.Install(iGate1);
  ndn::AppHelper producerHelper1("ns3::ndn::Producer");
  producerHelper1.SetAttribute("PayloadSize", StringValue("1024"));
  ndn::AppHelper gatewayHelper1("CustomApp");
  gatewayHelper1.Install(iGate2);

  //producer
  producerHelper.SetPrefix("/domain1/src1");
  producerHelper.Install(node1);
  producerHelper.SetPrefix("/domain1/test");
  producerHelper.Install(node1);
  producerHelper.SetPrefix("/domain1/src2");
  producerHelper.Install(node2);
  producerHelper.SetPrefix("/domain1/iGate1");
  producerHelper.Install(iGate1);
  producerHelper.SetPrefix("/domain2/dst1");
  producerHelper.Install(node3);
  producerHelper.SetPrefix("/domain2/dst2");
  producerHelper.Install(node4);
  producerHelper.SetPrefix("/domain2/dst3");
  producerHelper.Install(node5);
  producerHelper.SetPrefix("/domain2/iGate2");
  producerHelper.Install(iGate2);




  //manually configure FIB
  ndn::FibHelper::AddRoute(iGate2, "/domain2/dst3", node5, 20);
  ndn::FibHelper::AddRoute(iGate2, "/domain2/dst1", node3, 20);
  ndn::FibHelper::AddRoute(iGate2,"/domain2/dst2",node3,20);
  ndn::FibHelper::AddRoute(node3, "/domain2/iGate2", iGate2, 20);
  ndn::FibHelper::AddRoute(node3, "/domain2/dst2", node4, 20);
  ndn::FibHelper::AddRoute(node3, "/domain2/dst3", iGate2, 20);
  ndn::FibHelper::AddRoute(node4, "/domain2/dst1", node3, 20);
  ndn::FibHelper::AddRoute(node4, "/domain2/dst3", node5, 20);
  ndn::FibHelper::AddRoute(node4, "/domain2/iGate2", node3, 20);
  ndn::FibHelper::AddRoute(node5, "/domain2/dst2", node4, 20);
  ndn::FibHelper::AddRoute(node5, "/domain2iGate2", iGate2, 20);
  ndn::FibHelper::AddRoute(node5, "/domain2/dst1", iGate2, 20);
  ndn::FibHelper::AddRoute(node1, "/domain1/src2", iGate1, 20);
  ndn::FibHelper::AddRoute(node1, "/domain1/iGate1", iGate1, 20);
  ndn::FibHelper::AddRoute(node2, "/domain1/iGate1", iGate1, 20);
  ndn::FibHelper::AddRoute(node2, "/domain1/test", iGate1, 20);
  ndn::FibHelper::AddRoute(node2, "/domain1/src1", iGate1, 20);
  ndn::FibHelper::AddRoute(iGate1, "/domain1/src1", node1, 20);
  ndn::FibHelper::AddRoute(iGate1, "/domain1/test", node1, 20);
  ndn::FibHelper::AddRoute(iGate1, "/domain1/src2", node2, 20);

/*
  //Initiate three consumer application.
  //Set the events schedule in the consumer app
  //ndn::AppHelper consumerHelper0("ns3::ndn::ConsumerBatches");
  //consumerHelper0.SetAttribute("Batches", StringValue("7s 1"));
  //comsumer
  //consumerHelper0.SetPrefix("/dsxt2");
  //consumerHelper0.Install(node1);
  */

 //don't sent before 1 s second, i use this for gateway to send the election message
  ndn::AppHelper consumerHelper1("ns3::ndn::ConsumerBatches");
  consumerHelper1.SetAttribute("Batches", StringValue("3s 2"));
  //comsumer
  //consumerHelper1.SetPrefix("/tunnel/request");
  //consumerHelper1.Install(node2);
  consumerHelper1.SetPrefix("/domain2/dst2");
  consumerHelper1.Install(node5);
  ndn::AppHelper consumerHelper2("ns3::ndn::ConsumerBatches");
  //consumerHelper1.SetAttribute("Batches", StringValue("6s 2"));
  //consumerHelper1.SetPrefix("/tunnel/src2");
  //consumerHelper1.Install(node1);

  




  Ptr <Packet> packet1 = Create <Packet> (8000);
  //Simulator::Schedule (Seconds (3), &SimpleUdpApplication::SendPacket, udp0, packet1,dest_ip_ip5 , 7777);  

  LogComponentEnable ("SimpleUdpApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("GatewayTunnelStrategy", LOG_LEVEL_INFO);
  LogComponentEnable ("CustomApp", LOG_LEVEL_INFO);
  //LogComponentEnable ("Strategy", LOG_LEVEL_INFO);

  Simulator::Stop(Seconds(10.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}