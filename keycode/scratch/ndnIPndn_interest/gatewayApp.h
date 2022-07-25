
#ifndef CUSTOM_APP_H_
#define CUSTOM_APP_H_

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/socket.h"

namespace ns3 {

/**
 * @brief A simple custom application
 *
 * This applications demonstrates how to send Interests and respond with Datas to incoming interests
 *
 * When application starts it "sets interest filter" (install FIB entry) for /prefix/sub, as well as
 * sends Interest for this prefix
 *
 * When an Interest is received, it is replied with a Data with 1024-byte fake payload
 */
class CustomApp : public ndn::App {
public:
  // register NS-3 type "CustomApp"
  static TypeId
  GetTypeId();

   CustomApp ();
      virtual ~CustomApp ();

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

  

  //Ip function
  
      /** \brief handles incoming packets on port 7777
       */
      void HandleReadOne (Ptr<Socket> socket);

      /** \brief handles incoming packets on port 9999
       */
      void HandleReadTwo (Ptr<Socket> socket);

      /** \brief Send an outgoing packet. This creates a new socket every time (not the best solution)
      */
      void SendPacket (Ptr<Packet> packet, Ipv4Address destination, uint16_t port);


private:
  void
  SendInterest();
   void SetupReceiveSocket (Ptr<Socket> socket, uint16_t port);
      Ptr<Socket> m_recv_socket1; /**< A socket to receive on a specific port */
      Ptr<Socket> m_recv_socket2; /**< A socket to receive on a specific port */
      uint16_t m_port1; 
      uint16_t m_port2;

      Ptr<Socket> m_send_socket; /**< A socket to listen on a specific port */
};

} // namespace ns3

#endif // CUSTOM_APP_H_