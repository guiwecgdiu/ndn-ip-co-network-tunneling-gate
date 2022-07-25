#ifndef GATEWAYTUNNELSTRATEGY_HPP
#define GATEWAYTUNNELSTRATEGY_HPP

#include <boost/random/mersenne_twister.hpp>
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"


//When modify this file, plz remember to modifer the same file under the 
//fw dirct
namespace nfd {
namespace fw {

class GatewayTunnelStrategy : public Strategy {
public:
  GatewayTunnelStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  virtual
  ~GatewayTunnelStrategy() override;

  void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  
  

  static const Name&
  getStrategyName();

protected:
  boost::random::mt19937 m_randomGenerator;
};

} // namespace fw
} // namespace nfd

#endif // NDNSIM_EXAMPLES_NDN_LOAD_BALANCER_RANDOM_LOAD_BALANCER_STRATEGY_HPP