// custom-app.hpp

#ifndef GATEWAY_APP_H_
#define GATEWAY_APP_H_


#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/socket.h"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "gtt.hpp"


//new Jul 26
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/ndnSIM/ndn-cxx/name.hpp"
namespace ns3 {


class GatewayApp : public ndn::App {
public:
  // register NS-3 type "CustomApp"
  static TypeId
  GetTypeId();

  // (overridden from ndn::App) Processing upon start of the application
  virtual void
  StartApplication();

  // (overridden from ndn::App) Processing when application is stopped
  virtual void
  StopApplication();

  // (overridden from ndn::App) Callback that will be called when Interest arrives
  virtual void
  OnInterest(std::shared_ptr<const ndn::Interest> interest);

  // (overridden from ndn::App) Callback that will be called when Data arrives
  virtual void
  OnData(std::shared_ptr<const ndn::Data> contentObject);


////////////////////////////////////////////
//////////////////////////////////////////////
  //ip

      /** \brief handles incoming packets on port 7777
       */
      void HandleReadOne (Ptr<Socket> socket);

      /** \brief handles incoming packets on port 9999
       */
      void HandleReadTwo (Ptr<Socket> socket);

      /** \brief handles incoming packets on port tunnel
       */
      void HandleReadTunnelPort (Ptr<Socket> socket);

      /** \brief Send an outgoing packet. This creates a new socket every time (not the best solution)
      */
      void SendPacket (Ptr<Packet> packet, Ipv4Address destination, uint16_t port);

      /** \brief Send an outgoing packet. This creates a new socket every time (not the best solution)
      */
      void BuildTunnel(Ipv4Address destination,uint16_t port);


      void BeforeTunnelBuild(Ptr<Socket> socket);

/////////////////////////////////////////////////
//Gtttable
///////////////////////////////////////////////////

    void 
    Gtt_addRoute(ndn::Name prefix, Ipv4Address ipv4address);

/*
 
  GttTable g = GttTable();
  
  int size=interest->getName().size();
  g.AddRoute(interest->getName().getPrefix(size-1),"10.1.5.2");
  g.AddRoute(ndn::Name("/app"),"10.1.5.2");
  g.AddRoute(interest->getName().getPrefix(size-1),"10.1.5.1");
  g.AddRoute(ndn::Name("/app2"),"10.1.5.1");
  std::string ip= g.mapToGateIP(ndn::Name("/app2"));
  cout<<PURPLE_CODE<<"the ip is "<<ip<<" "<<END_CODE<<std::endl;
  g.printTheMap();
  

*/

private:
  void
  SendInterest();

  void
  ReformAndSendInterest(std::shared_ptr<ndn::Interest> interest);


  void
  ReformAndSendData(std::shared_ptr<ndn::Data> Data);

  void
  SendElection();
  

  void 
  SetupReceiveSocket (Ptr<Socket> socket, uint16_t port);

  Ptr<Socket> m_recv_socket1; /**< A socket to receive on a specific port */
  Ptr<Socket> m_recv_socket2; /**< A socket to receive on a specific port */
  Ptr<Socket> m_recv_socketTunnel;
  Ptr<Socket> m_Tunnel_echo_socket;
  uint16_t m_port1; 
  uint16_t m_port2;
  uint16_t m_tunnelPort;
  Ptr<Socket> m_send_socket; /**< A socket to listen on a specific port */
  GttTable m_gtt;
  GttTable m_dtt;


  vector<int> m_available_port{ 7007, 6006, 8008,9009, 3003,2002,1001};
  //producer set jul 26
  ndn::Name m_prefix;
  ndn::Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  uint32_t m_signature;
  ndn::Name m_keyLocator;


  //gtt table

};
} // namespace ns3

#endif // CUSTOM_APP_H_