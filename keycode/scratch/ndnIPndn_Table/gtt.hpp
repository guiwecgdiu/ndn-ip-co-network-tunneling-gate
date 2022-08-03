#include "ns3/ndnSIM/ndn-cxx/name.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
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
    AddRoute(ndn::Name prefix, std::string ipv4address);
    void 
    RemoveRoute(ndn::Name prefix,std::string ipv4address);
    bool
    HasRoute(ndn::Name prefix,std::string ipv4address);
    void
    printTheMap();
    std::string
    mapToGateIP(ndn::Name prefix);

private:
    int m_value = 0;
    std::map<std::string,std::vector<ndn::Name>> m_GttMap;
    std::string findIpByPrefix(ndn::Name name);


};



