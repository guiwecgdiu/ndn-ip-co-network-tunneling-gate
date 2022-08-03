#ifndef NDN_UDP_FACE_H
#define NDN_UDP_FACE_H
#include "ns3/ndnSIM-module.h"


#include "ns3/ndnSIM/NFD/daemon/face/face.hpp"
#include "ns3/socket.h"
#include "ns3/ptr.h"

#include <map>

namespace ns3{
namespace ndn{

class UdpFace : public enable_shared_from_this<Face>
{
   public:
      static TypeId
      GetTypeId ();
   
      UdpFace (Ptr<Node> node, Ptr<Socket> socket, Ipv4Address address);
      virtual ~UdpFace();
 
      Ipv4Address GetAddress () const;

      virtual bool ReceiveFromUdp (Ptr<const Packet> p);

   protected:
      // also from ndn::Face
      virtual bool
      Send (Ptr<Packet> p);

   private:  
      UdpFace (const UdpFace &); 
      UdpFace& operator= (const UdpFace &); 
 

   private:
      Ptr<Socket> m_socket;
      Ipv4Address m_address;
      Ptr<Socket> m_Node;
};




}//namespace ndn
}//namespace ns3
#endif // NDN_UDP_FACE_H