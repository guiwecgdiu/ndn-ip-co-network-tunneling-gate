#include "ns3/header.h"

namespace ns3 {


class TIPHeader : public Header
{
public:
  // must be implemented to become a valid new header.
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  // allow protocol-specific access to the header data.
  void SetData (uint32_t data);
  uint32_t GetData (void) const;
private:
  uint32_t m_data;
};

}