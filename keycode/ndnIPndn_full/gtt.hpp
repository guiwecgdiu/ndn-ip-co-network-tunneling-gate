#include "ns3/ndnSIM/ndn-cxx/name.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include <map>
#include <vector>
#include <string>
#include <utility>

// https://codereview.stackexchange.com/questions/211826/code-to-read-and-write-csv-files
using namespace std;

class GttTable
{

public:
    GttTable();
    void 
    AddRoute(ndn::Name prefix, ns3::Ipv4Address ipv4address);
    void 
    RemoveRoute(ndn::Name prefix,ns3::Ipv4Address ipv4address);
    bool
    HasRoute(ndn::Name prefix,ns3::Ipv4Address ipv4address);
    void
    printTheMap();

    void
    printDTTMap();
    ns3::Ipv4Address
    mapToGateIP(ndn::Name prefix);

private:
    int m_value = 0;
    std::map<ns3::Ipv4Address,std::vector<ndn::Name>> m_GttMap;
    std::string findIpByPrefix(ndn::Name name);


};



