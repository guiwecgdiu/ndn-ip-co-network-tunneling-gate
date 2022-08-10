#include "gatewayTunnelStrategy.hpp"

#include <boost/random/uniform_int_distribution.hpp>

#include <ndn-cxx/util/random.hpp>
#include "ndn-cxx/name.hpp"

#include "daemon/common/logger.hpp"
#include <vector>

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



namespace nfd {
namespace fw {
  
NFD_REGISTER_STRATEGY(GatewayTunnelStrategy);
NS_LOG_COMPONENT_DEFINE("GatewayTunnelStrategy");


GatewayTunnelStrategy::GatewayTunnelStrategy(Forwarder& forwarder, const Name& name)
  : BestRouteStrategy(forwarder)
{
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
  
}

GatewayTunnelStrategy::~GatewayTunnelStrategy()
{
}


const Name&
GatewayTunnelStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/gateway-tunnel-strategy/%FD%03r/%FD%03");
  return strategyName;
}


//Triggers
void
GatewayTunnelStrategy::afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                                                 const shared_ptr<pit::Entry>& pitEntry)
{


  //register for  the ndnGateway Application
  if(interest.getName().toUri()=="/tunnel/tunnelRegisty")
  {
     NS_LOG_INFO(RED_CODE << "call after receive registy"  << END_CODE);
    m_RegistyApp=interest.getName();
    NS_LOG_INFO(BLUE_CODE<<"Receive a gateway App registry interest"<<END_CODE);
    return;
  }
  

  //JUL 30 Arno,
  //1. Read the incoming interest to get weither it a in domain traffic or out domain traffic
  //2. case 1: in domain traffic(it means it is included in the FIB forward as fib)
  //3.case 2, out-domain traffic (it means it is excluded in the FIB forward while it is included in the GTT)
  //Components: 1.GTT 2.FIB 3.flow chart
  
  //####
  //Case 1 weither it is in the fib
  NS_LOG_INFO(GREEN_CODE<<interest.toUri()<<END_CODE);
  //inDomainTraffic(interest,ingress,pitEntry);

  
  
  if(hasPrefixInDomain(interest)){
    NS_LOG_INFO(GREEN_CODE<<"The check with result True,in domain"<<END_CODE);
    inDomainTraffic(interest,ingress,pitEntry);
  }else{
    NS_LOG_INFO(GREEN_CODE<<"The check with result False,out domain"<<END_CODE);
    outDomainTraffic(interest,ingress,pitEntry);
  
  }
  
  
  
}





void
GatewayTunnelStrategy::afterReceiveData(const Data& data, const FaceEndpoint& ingress,
                                            const shared_ptr<pit::Entry>& pitEntry)
{
  BestRouteStrategy::afterReceiveData(data,ingress,pitEntry);

}


void
GatewayTunnelStrategy::afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                                                    const shared_ptr<pit::Entry>& pitEntry)
{
  BestRouteStrategy::afterReceiveNack(nack,ingress,pitEntry);
}  



//Forwarding logic on indomian-> Bestroute process traffic

void
GatewayTunnelStrategy::inDomainTraffic(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry)
{
  BestRouteStrategy::afterReceiveInterest(interest,ingress,pitEntry);
}


//Forwarding logic out domian logic->IP tunnel logics
void
GatewayTunnelStrategy::outDomainTraffic(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry)
{
  m_outdomainList.push_back(interest.getName().getSubName(0,interest.getName().size()-1));
  NS_LOG_INFO(BLUE_CODE<<"Receive a gateway App tunnel interest"<<END_CODE);
  NS_LOG_INFO(BLUE_CODE<< m_RegistyApp <<END_CODE);
    //m_forwarder.onIncomingInterest(interest,)
  const nfd::fib::Entry& output_entry= m_forwarder.getFib().findLongestPrefixMatch(m_RegistyApp);
  const fib::NextHopList& nexthops=output_entry.getNextHops();
  NS_LOG_INFO(BLUE_CODE<<"The face is in record "<< output_entry.getPrefix()<<END_CODE);
    
  for (const auto& nexthop : nexthops) {
    Face& outFace = nexthop.getFace();
  NS_LOG_INFO("send ndn-discovery Interest=" << interest.toUri() << " from=" << ingress.face.getId() <<" to=" << outFace.getId());
  this->sendInterest(interest, outFace, pitEntry);
  }

}

//Outdomian decider
bool 
GatewayTunnelStrategy::hasPrefixInDomain(const Interest& interest)
{
  nfd::fib::Fib::const_iterator i;

  /*
  for(auto it = m_outdomainList.begin(); it != m_outdomainList.end(); it++){
    // found nth element..print and break.
    if(interest.getName().getSubName(0,interest.getName().size()-1) == it->getSubName(0,it->size())) {
        NS_LOG_INFO(YELLOW_CODE<<"In the outdomain list"<<END_CODE);
    }else{
        NS_LOG_INFO(YELLOW_CODE<<"Out the outdomain list"<<END_CODE);
    }
  }
  */
  

  for(i=m_forwarder.getFib().begin();i!=m_forwarder.getFib().end();i++)
  { 
    //compare the interest with the fib on their prefix
    //use subname to remove the sequence
    ndn::Name n,a;
    n=interest.getName().getSubName(0,interest.getName().size()-1);
    a=i->getPrefix();
    NS_LOG_INFO(YELLOW_CODE<<"Compare at "<<n<<" vs "<<a<<END_CODE);
    if(interest.getName().getSubName(0,interest.getName().size()-1)==  i->getPrefix())
    {
      return true;
    }
  }

  

/*
#for test print all prefix in fib 
for (ndn::Name n: appendList)
    std::cout << n << ' ';
  
std::cout <<" "<<std::endl;
*/
  return false;
}



} // namespace fw
} // namespace nfd