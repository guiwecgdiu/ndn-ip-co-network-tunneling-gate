// ndn-congestion-topo-plugin.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/internet-stack-helper.h"

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/ndn-ip-ndn.txt");
  topologyReader.Read();

  //Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "10000");
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  


   // Getting containers for the consumer/producer
 
   Ptr<Node> node1 = Names::Find<Node>("Src1");
  Ptr<Node> node2 = Names::Find<Node>("Src2");
  Ptr<Node> node3 = Names::Find<Node>("Dst1");
  Ptr<Node> node4 = Names::Find<Node>("Dst2");
	Ptr<Node> Iprouter1 = Names::Find<Node>("Rtr2");
	Ptr<Node> Iprouter2 = Names::Find<Node>("Rtr3");
	Ptr<Node> iGate1 = Names::Find<Node>("Rtr1");
	Ptr<Node> iGate2 = Names::Find<Node>("Rtr4");



	
  // Install NDN stack on ndn node
  //ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "10000");
  //ndnHelper.Install(consumer1);
	//ndnHelper.Install(consumer2);
	//ndnHelper.Install(producer1);
	//ndnHelper.Install(producer2);
	//ndnHelper.Install(iGate1);
	//ndnHelper.Install(iGate2);

  //install ipv4 stack on ip nodes
  //ndnHelper.Install(Iprouter1);
	//ndnHelper.Install(Iprouter2);



  //Installing global routing interface on all nodes
  //ndnGlobalRoutingHelper.Install(consumer1);
  //ndnGlobalRoutingHelper.Install(consumer2);
  //ndnGlobalRoutingHelper.Install(producer1);
  //ndnGlobalRoutingHelper.Install(producer2);
  //ndnGlobalRoutingHelper.Install(iGate1);
  //ndnGlobalRoutingHelper.Install(iGate2);

  
	
	

	


  //ipv4 address
  //Ipv4AddressHelper address;



  // Getting containers for the consumer/producer
  //Ptr<Node> consumer1 = Names::Find<Node>("Src1");
  //Ptr<Node> consumer2 = Names::Find<Node>("Src2");

  //Ptr<Node> producer1 = Names::Find<Node>("Dst1");
  //Ptr<Node> producer2 = Names::Find<Node>("Dst2");

  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //consumerHelper.SetAttribute("Frequency", StringValue("100")); // 100 interests a second

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  consumerHelper.SetAttribute("Batches", StringValue("1s 2"));

  // on the first consumer node install a Consumer application
  // that will express interests in /dst1 namespace

  //consumerHelper.SetPrefix("/dst1");
  //consumerHelper.Install(consumer1);
  consumerHelper.SetPrefix("/dst1");
  consumerHelper.Install(node1);


  // on the second consumer node install a Consumer application
  // that will express interests in /dst2 namespace
  //consumerHelper.SetPrefix("/dst2");
  //consumerHelper.Install(node2);

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

  // Register /dst1 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst1 namespace
  ndnGlobalRoutingHelper.AddOrigins("/dst1", node3);
  producerHelper.SetPrefix("/dst1");
  producerHelper.Install(node3);

  // Register /dst2 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst2 namespace
  //ndnGlobalRoutingHelper.AddOrigins("/dst2", producer2);
  //producerHelper.SetPrefix("/dst2");
  //producerHelper.Install(producer2);

  // Calculate and install FIBs
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