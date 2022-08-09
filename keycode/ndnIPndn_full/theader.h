#include "ns3/header.h"

namespace ns3 {


class THeader : public Header
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
  void SetData (uint16_t data);
  uint16_t GetData (void) const;
private:
  uint16_t m_data;
};

}