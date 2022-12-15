#ifndef GATEWAYTUNNELSTRATEGY_HPP
#define GATEWAYTUNNELSTRATEGY_HPP

#include <boost/random/mersenne_twister.hpp>
#include "best-route-strategy.hpp"
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"

//When modify this file, plz remember to modifer the same file under the 
//fw dirct
namespace nfd {
namespace fw {

class GatewayTunnelStrategy : public BestRouteStrategy {
public:
  GatewayTunnelStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

  virtual
  ~GatewayTunnelStrategy() override;


  public: // triggers
  void
  afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveData(const Data& data, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveNack(const lp::Nack& nack, const FaceEndpoint& ingress,
                   const shared_ptr<pit::Entry>& pitEntry) override;


  void
  inDomainTraffic(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry);

  void
  outDomainTraffic(const Interest& interest, const FaceEndpoint& ingress,
                       const shared_ptr<pit::Entry>& pitEntry);

  bool
  hasPrefixInDomain(const Interest& interest);

protected:
  boost::random::mt19937 m_randomGenerator;
  Name m_RegistyApp;
  std::vector<ndn::Name> m_outdomainList;


};

} // namespace fw
} // namespace nfd

#endif // NDNSIM_EXAMPLES_NDN_LOAD_BALANCER_RANDOM_LOAD_BALANCER_STRATEGY_HPP