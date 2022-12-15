
#include "tnumheader.h"
using namespace ns3;

TypeId
TNUMHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("TNUMHeader")
    .SetParent<Header> ()
    .AddConstructor<TNUMHeader> ()
  ;
  return tid;
}

uint32_t 
TNUMHeader::GetSerializedSize (void) const
{
  return 6;
}

void 
TNUMHeader::Serialize (Buffer::Iterator start) const
{
  // The 2 byte-constant
  start.WriteU8 (0xfe);
  start.WriteU8 (0xef);
  // The data.
  start.WriteHtonU32 (m_data);
}


uint32_t 
TNUMHeader::Deserialize (Buffer::Iterator start)
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
TNUMHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}


void 
TNUMHeader::Print (std::ostream &os) const
{
  os << "data=" << m_data;
  
}


 
void
TNUMHeader::SetData (uint32_t data)
{
  m_data = data;
}
uint32_t
TNUMHeader::GetData (void) const
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