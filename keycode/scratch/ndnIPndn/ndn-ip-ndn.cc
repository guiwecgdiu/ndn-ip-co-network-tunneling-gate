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



using namespace ns3;


namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);
  


/*
  //Read the ndn domain topologies
  //load the topologies
  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/ndn-ip-ndn.txt");
  topologyReader.Read();
  


   // Getting containers for the consumer/producer
  Ptr<Node> node1 = Names::Find<Node>("Src1");
  Ptr<Node> node2 = Names::Find<Node>("Src2");
  Ptr<Node> node3 = Names::Find<Node>("Dst1");
  Ptr<Node> node4 = Names::Find<Node>("Dst2");
  Ptr<Node> node5 = Names::Find<Node>("Dst3");
	Ptr<Node> iGate1 = Names::Find<Node>("Rtr1");
	Ptr<Node> iGate2 = Names::Find<Node>("Rtr4");
  */
  

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



	
  // Install NDN stack on ndn node
  ndn::StackHelper ndnHelper;
  ndnHelper.Install(node1);
	ndnHelper.Install(node2);
	ndnHelper.Install(node3);
	ndnHelper.Install(node4);
  ndnHelper.Install(node5);
	ndnHelper.Install(iGate1);
	ndnHelper.Install(iGate2);

  
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;

  ndnGlobalRoutingHelper.Install(node1);
	ndnGlobalRoutingHelper.Install(node2);
  ndnGlobalRoutingHelper.Install(iGate1);
  ndnGlobalRoutingHelper.Install(node3);
	ndnGlobalRoutingHelper.Install(node4);
  ndnGlobalRoutingHelper.Install(node5);
  ndnGlobalRoutingHelper.Install(iGate2);
	
// Choosing forwarding strategy
  ndn::StrategyChoiceHelper::Install(node1,"/prefix", "/localhost/ndf/strategy/bestroute");
	ndn::StrategyChoiceHelper::Install(node2,"/prefix", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(iGate1,"/prefix", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(iGate2,"/prefix", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(node3,"/prefix", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(node4,"/prefix", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(node5,"/prefix", "/localhost/nfd/strategy/best-route");
  


  //Initiate three consumer application.
  //Set the events schedule in the consumer app
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  consumerHelper.SetAttribute("Batches", StringValue("3s 2"));
  ndn::AppHelper consumerHelper2("ns3::ndn::ConsumerBatches");
  consumerHelper2.SetAttribute("Batches", StringValue("4s 2 7s 5"));
  ndn::AppHelper consumerHelper3("ns3::ndn::ConsumerBatches");
  consumerHelper3.SetAttribute("Batches", StringValue("7s 10"));



  // on the first consumer node (node1) install a Consumer application
  // that will express interests in /src2 namespace
  consumerHelper.SetPrefix("/src2");
  consumerHelper.Install(node1);

  // on the second consumer node (iGate2) install a Consumer application
  // it will express interest in /dst2 namesapce
  consumerHelper2.SetPrefix("/dst2");
  consumerHelper2.Install(iGate2);

  //on the third cosumer node (node 5) install a consumer application
  //it will express interest in /Rtr1
  consumerHelper3.SetPrefix("/Rtr1");
  consumerHelper3.Install(node1);
  
  //Initoate the producer app
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));


  // Register /src2 prefix with global routing controller and
  // install producer that will satisfy Interests in /src2 namespace
  //install the /src producer in node2
  ndnGlobalRoutingHelper.AddOrigins("/src2", node2);
  producerHelper.SetPrefix("/src2");
  producerHelper.Install(node2);

  // Register /dst2 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst2 namespace
  //install the /src producer in node4
  ndnGlobalRoutingHelper.AddOrigins("/dst2", node4);
  producerHelper.SetPrefix("/dst2");
  producerHelper.Install(node4);


  // Register /Rtr4 prefix with global routing controller and
  // install producer that will satisfy Interests in /Rtr4 namespace
  //install the /src producer in iGate2
  ndnGlobalRoutingHelper.AddOrigins("/Rtr4", iGate2);
  producerHelper.SetPrefix("/Rtr4");
  producerHelper.Install(iGate2);
  
  // Register /Rtr1 prefix with global routing controller and
  // install producer that will satisfy Interests in /Rtr1 namespace
  //install the /src producer in iGate1
  ndnGlobalRoutingHelper.AddOrigins("/Rtr1",iGate1);
  producerHelper.SetPrefix("/Rtr1");
  producerHelper.Install(iGate1);

   // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();



  


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
  NodeContainer central;
  int j=0;
  for(j = 1; j < 3; j++)
  {   
    central.Add (nodes.Get(j));
  }

  NodeContainer border1;
  j=0;
  for(j = 0; j < 2; j++)
  {   
    border1.Add (nodes.Get(j));
  }

  NodeContainer border2;
  j=0;
  for(j = 2; j < 4; j++)
  {   
    border2.Add (nodes.Get(j));
  }

  
  
 
  //Build links in each domain, csmaDevs for centrial, csmaDevs1 for border 1, csmaDev2 for border 2
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate",StringValue("20Mbps"));
  csma.SetChannelAttribute("Delay",TimeValue(NanoSeconds(6560)));

  NetDeviceContainer csmaDevs,csmaDevs1,csmaDevs2,csmaDevs3,csmaDevs4;
  csmaDevs =csma.Install(central);
  csma.EnableAsciiAll("ndnIPCentral");
  csmaDevs1 =csma.Install(border1);
  csma.EnableAsciiAll("ndnIPBorder1");
  csmaDevs2 =csma.Install(border2);
  csma.EnableAsciiAll("ndnIPBorder2");


  //install the network stacks.to emplty nodes
  InternetStackHelper stack;
  stack.Install(nodes);
  


  //assign the Ip address for network devices, central for 10.1.1.0
  //border 1 for 10.1.2.0
  //border 2 for 10.1.3.0
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces;
  ifaces = address.Assign (csmaDevs);
  address.SetBase("10.1.2.0","255.255.255.0");
  Ipv4InterfaceContainer ifaces1;
  ifaces1 = address.Assign (csmaDevs1);
  address.SetBase("10.1.3.0","255.255.255.0");
  Ipv4InterfaceContainer ifaces2;
  ifaces2 = address.Assign (csmaDevs2);

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
  nodes.Get(0)->AddApplication (udp0);
  nodes.Get(1)->AddApplication (udp1);
  nodes.Get(2)->AddApplication (udp2);
  nodes.Get(3)->AddApplication (udp3);
 

  //TRead from the topologies and explict the ip address for each nodes, in a same index.
  Ipv4Address dest_ip_ip0 ("10.1.2.1");
  Ipv4Address dest_ip_ip1 ("10.1.1.1");
  Ipv4Address dest_ip_ip2 ("10.1.1.2");
  Ipv4Address dest_ip_ip3 ("10.1.3.2");

  /*
  //Schedule an event to send a packet of size 400 using udp0 targeting IP of node 0, and port 7777
  Ptr <Packet> packet1 = Create <Packet> (400);
  Simulator::Schedule (Seconds (1), &SimpleUdpApplication::SendPacket, udp1, packet1, dest_ip_ip3, 7777);  
   //Another packet of size 800, targeting the same IP address, but a different port.
  Ptr <Packet> packet2 = Create <Packet> (800);
  Simulator::Schedule (Seconds (2), &SimpleUdpApplication::SendPacket, udp0, packet2, dest_ip_ip3, 9999);

  LogComponentEnable ("SimpleUdpApplication", LOG_LEVEL_INFO);
*/


  //Hybrid node section,connect the NDN border router with the IP backbone
  //Through a point to point link
  // Install NDN stack on all nodes
  ns3::Ptr<Node> ip0,ip3;
  ip0=nodes.Get(0);
  ip3=nodes.Get(3);
   //Make a ip node to hybrid nodes
  //install ndn stack



  //ns3::PointToPointHelper p2p;
  //p2p.Install(iGate1,ip0);
  //p2p.Install(ip3, iGate2);






  
  

  
  
  
  


  NodeContainer hybrid1, hybrid2;
  
  csma.EnableAsciiAll("ndnIPHybrid1");
  hybrid1.Add(iGate1);
  hybrid1.Add(ip0);
  stack.Install(iGate1);
  csmaDevs3 =csma.Install(hybrid1);
  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces3;
  ifaces3 = address.Assign (csmaDevs3);

  csma.EnableAsciiAll("ndnIPHybrid2");
  hybrid2.Add(ip3);
  hybrid2.Add(iGate2);
  stack.Install(iGate2);
  csmaDevs4 =csma.Install(hybrid2);
  address.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces4;
  ifaces4 = address.Assign (csmaDevs4);
  iGate1->AddApplication(udp4);
  iGate2->AddApplication(udp5);
  Ipv4Address dest_ip_ig1 ("10.1.4.1");
  Ipv4Address dest_ip_ig2 ("10.1.5.2");
  
  Ipv4Address dest_ip_bd1 ("10.1.4.2");
  Ipv4Address dest_ip_bd2 ("10.1.5.1");


  

  //ndnGlobalRoutingHelper.AddOrigin("/Hybrid0",ip0);
  //producerHelper.SetPrefix("/Hybrid0");
  //producerHelper.Install(ip0);

  //ndnGlobalRoutingHelper.AddOrigin("Hybrid1",ip3);
  //producerHelper.SetPrefix("/Hybrid1");
  //producerHelper.Install(ip3);


  //registe them as cosnumer and trigger event
  



  //Hybrid node section
  

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  Packet::EnablePrinting();
  //Schedule an event to send a packet of size 400 using udp0 targeting IP of node 0, and port 7777
  Ptr <Packet> packet1 = Create <Packet> (400);
  Simulator::Schedule (Seconds (3), &SimpleUdpApplication::SendPacket, udp3, packet1, dest_ip_ip0, 7777);  
   //Another packet of size 800, targeting the same IP address, but a different port.
  Ptr <Packet> packet2 = Create <Packet> (800);
  Simulator::Schedule (Seconds (2), &SimpleUdpApplication::SendPacket, udp0, packet2, dest_ip_ip3, 9999);

  LogComponentEnable ("SimpleUdpApplication", LOG_LEVEL_INFO);


  //


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