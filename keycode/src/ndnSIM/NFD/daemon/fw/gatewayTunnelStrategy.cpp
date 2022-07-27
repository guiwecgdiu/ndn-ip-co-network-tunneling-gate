#include "gatewayTunnelStrategy.hpp"

#include <boost/random/uniform_int_distribution.hpp>

#include <ndn-cxx/util/random.hpp>

#include "daemon/common/logger.hpp"

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
  : Strategy(forwarder)
{
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

GatewayTunnelStrategy::~GatewayTunnelStrategy()
{
}

static bool
canForwardToNextHop(const Face& inFace, shared_ptr<pit::Entry> pitEntry, const fib::NextHop& nexthop)
{
  return !wouldViolateScope(inFace, pitEntry->getInterest(), nexthop.getFace());
}

static bool
hasFaceForForwarding(const Face& inFace, const fib::NextHopList& nexthops, const shared_ptr<pit::Entry>& pitEntry)
{
  return std::find_if(nexthops.begin(), nexthops.end(), bind(&canForwardToNextHop, cref(inFace), pitEntry, _1))
         != nexthops.end();
}

void
GatewayTunnelStrategy::afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                                                 const shared_ptr<pit::Entry>& pitEntry)
{
 //JUL 25 I use prefix to identify the outgoing traffic 
  NS_LOG_INFO(PURPLE_CODE << "call after receive Interest"  << END_CODE);
  std::string name=interest.getName().getPrefix(-1).toUri();
  NS_LOG_INFO(PURPLE_CODE << name  << END_CODE);
  //int size = m_forwarder.onIncomingData();

  //JUL 27 00:16 ARNO, edit for judge weither a interest is 
  //the Gateway app registry information
  
  if(interest.getName().toUri()=="/tunnel/tunnelRegisty")
  {
    m_RegistyApp=interest.getName();
    NS_LOG_INFO(BLUE_CODE<<"Receive a gateway App registry interest"<<END_CODE);
  }

  if(name=="/tunnel")
  {
    NS_LOG_INFO(BLUE_CODE<<"Receive a gateway App tunnel interest"<<END_CODE);
    NS_LOG_INFO(BLUE_CODE<< m_RegistyApp <<END_CODE);
    
    //m_forwarder.onIncomingInterest(interest,)
    
     
  }
  
  
  


  //NS_LOG_INFO(PURPLE_CODE << "the size of the FIB is"<< size  << END_CODE);

  if (hasPendingOutRecords(*pitEntry)) {
    // not a new Interest, don't forward
    return;
  }
  
  

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();
  
/*
  if(fibEntry.hasNextHops()){
    NS_LOG_INFO(PURPLE_CODE << fibEntry.getPrefix().toUri()  << END_CODE);
  }else{
    NS_LOG_INFO(PURPLE_CODE << fibEntry.getPrefix().toUri()  << END_CODE);
  }*/

  // Ensure there is at least 1 Face is available for forwarding
  if (!hasFaceForForwarding(ingress.face, nexthops, pitEntry)) {
    this->rejectPendingInterest(pitEntry);
    return;
  }

  

  
  
  


  /*
  fib::NextHopList::const_iterator selected;
  do {
    boost::random::uniform_int_distribution<> dist(0, nexthops.size() - 1);
    const size_t randomIndex = dist(m_randomGenerator);

    uint64_t currentIndex = 0;

    for (selected = nexthops.begin(); selected != nexthops.end() && currentIndex != randomIndex;
         ++selected, ++currentIndex) {
    }
  } while (!canForwardToNextHop(ingress.face, pitEntry, *selected));
  NFD_LOG_TRACE("afterReceiveInterest");
  this->sendInterest(interest, selected->getFace(), pitEntry);// forward out
  this->sendInterest(interest, selected->getFace(), pitEntry);// forward out
  */
 
  
}

const Name&
GatewayTunnelStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/gateway-tunnel-strategy/%FD%03r/%FD%03");
  return strategyName;
}

} // namespace fw
} // namespace nfd