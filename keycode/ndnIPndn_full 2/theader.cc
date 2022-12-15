
#include "theader.h"
using namespace ns3;

TypeId
THeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("THeader")
    .SetParent<Header> ()
    .AddConstructor<THeader> ()
  ;
  return tid;
}

uint32_t 
THeader::GetSerializedSize (void) const
{
  return 6;
}

void 
THeader::Serialize (Buffer::Iterator start) const
{
  // The 2 byte-constant
  start.WriteU8 (0xfe);
  start.WriteU8 (0xef);
  // The data.
  start.WriteHtonU32 (m_data);
}


uint32_t 
THeader::Deserialize (Buffer::Iterator start)
{
  uint8_t tmp;
  tmp = start.ReadU8 ();
  NS_ASSERT (tmp == 0xfe);
  tmp = start.ReadU8 ();
  NS_ASSERT (tmp == 0xef);
  m_data = start.ReadNtohU32 ();
  return 6; // the number of bytes consumed.
}


TypeId 
THeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}


void 
THeader::Print (std::ostream &os) const
{
  os << "data=" << m_data;
  
}


 
void
THeader::SetData (uint16_t data)
{
  m_data = data;
}
uint16_t
THeader::GetData (void) const
{
  return m_data;
}
 

//Once this class is implemented, you can easily store your protocol header into a packet: 
/*

Ptr<Packet> p = ...;
YHeader yHeader;
yHeader.SetData (0xdeadbeaf);
// copy the header into the packet
p->AddHeader (yHeader);


// and get it out of a packet: 
Ptr<Packet> p = ...;
YHeader yHeader;
// copy the header from the packet
p->RemoveHeader (yHeader);
uint32_t data = yHeader.GetData ();


*/