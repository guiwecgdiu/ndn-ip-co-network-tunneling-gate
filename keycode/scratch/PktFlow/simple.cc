// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/internet-stack-helper.h"
#include "simpleUDPApp.h"
#include "ns3/ipv4-global-routing-helper.h"

namespace ns3 {

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  //Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3);
  InternetStackHelper stack;
  stack.Install(nodes);

  // Connecting nodes using two links
  NetDeviceContainer netDev1,netDev2;
  PointToPointHelper p2p;
  netDev1=p2p.Install(nodes.Get(0), nodes.Get(1));
  netDev2=p2p.Install(nodes.Get(1), nodes.Get(2));


  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces;
  address.Assign (netDev1);
  address.SetBase ("10.1.2.0", "255.255.255.0");
  address.Assign (netDev2);
  



  // Install NDN stack on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();
  ndnGlobalRoutingHelper.InstallAll();

    
  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  // Installing applications
    Ptr <SimpleUdpApplication> udp0 = CreateObject <SimpleUdpApplication> ();
  Ptr <SimpleUdpApplication> udp1 = CreateObject <SimpleUdpApplication> ();
  Ptr <SimpleUdpApplication> udp2 = CreateObject <SimpleUdpApplication> ();


   udp0->SetStartTime (Seconds(0));
  udp0->SetStopTime (Seconds (10));
  udp1->SetStartTime (Seconds(0));
  udp1->SetStopTime (Seconds (10));
  udp2->SetStartTime (Seconds(0));
  udp2->SetStopTime (Seconds (10));

    nodes.Get(0)->AddApplication (udp0);
  nodes.Get(1)->AddApplication (udp1);
  nodes.Get(2)->AddApplication (udp2);
  
   Ipv4Address dest_ip_ip0 ("10.1.1.1");
  Ipv4Address dest_ip_ip1 ("10.1.1.2");
  Ipv4Address dest_ip_ip2 ("10.1.2.2");


    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Ptr <Packet> packet1 = Create <Packet> (400);
  Simulator::Schedule (Seconds (1), &SimpleUdpApplication::SendPacket, udp0 , packet1, dest_ip_ip2, 7777);  
   //Another packet of size 800, targeting the same IP address, but a different port.
  Ptr <Packet> packet2 = Create <Packet> (800);
  Simulator::Schedule (Seconds (7), &SimpleUdpApplication::SendPacket, udp0, packet2, dest_ip_ip1, 9999);


    
  // Consumer
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  consumerHelper.SetAttribute("Batches", StringValue("10s 2"));
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  //consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests a second
  auto apps = consumerHelper.Install(nodes.Get(0));                        // first node
  //apps.Stop(Seconds(10.0)); // stop the consumer app at 10 seconds mark

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
 
  ndnGlobalRoutingHelper.AddOrigins("/Rtr1",nodes.Get(2));
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(2)); // last node
  ndn::GlobalRoutingHelper::CalculateRoutes();
  

  Simulator::Stop(Seconds(20.0));

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

