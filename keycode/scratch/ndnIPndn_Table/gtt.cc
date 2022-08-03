#include "gtt.hpp"
#define PURPLE_CODE "\033[95m"
#define CYAN_CODE "\033[96m"
#define TEAL_CODE "\033[36m"
#define BLUE_CODE "\033[94m"
#define GREEN_CODE "\033[32m"
#define YELLOW_CODE "\033[33m"
#define LIGHT_YELLOW_CODE "\033[93m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

GttTable::GttTable(){
    

}


std::string GttTable::mapToGateIP(ndn::Name name)
{
    
 for (auto x : m_GttMap) {
        if(HasRoute(name,x.first)){
            return x.first;
        }
    }
    return NULL;
}


void GttTable::AddRoute(ndn::Name name, std::string ip){


    if(m_GttMap.count(ip)) {//if ip is already in list
        std::vector<ndn::Name> names;
        auto it = m_GttMap.find(ip);
        names=it->second;

        //test the existence of the name
        bool hasName = false;
        for(auto & elem : names)
        {
            if(name==elem){
                hasName=true;
                break;
            }for(auto & elem : names)
        {
            if(name==elem){
                hasName=true;
                break;
            }
        }
        if(hasName==true){

        }else{
            names.push_back(name);
            m_GttMap.erase(ip);
            m_GttMap.insert(make_pair(ip,names));
        }
        }

    } else { //else ip is not already in list so add i
        std::vector<ndn::Name> names;
        names.push_back(name);
        m_GttMap.insert(make_pair(ip,names));
    }


}

bool GttTable::HasRoute(ndn::Name name, std::string ip){
    if(m_GttMap.count(ip)){//if the ip exist in the table
        std::vector<ndn::Name> names;
        auto it = m_GttMap.find(ip);
        names=it->second;
        for(auto & elem : names)
        {
            if(name==elem){
                return true;
            }
        }
        return false;
    } else{                      //if the ip not exist in the table
        return false;
    }
}

void GttTable::RemoveRoute(ndn::Name name, std::string ip){
    
    if(m_GttMap.count(ip)) {//if ip is already in list
        std::vector<ndn::Name> names;
        auto it = m_GttMap.find(ip);
        names=it->second;


        for(auto it = std::begin(names); it != std::end(names); ++it) {
               // std::cout << *it << "\n";
               if(name==*it)
               {    
                    names.erase(it);
               }
        }
            m_GttMap.erase(ip);
            m_GttMap.insert(make_pair(ip,names));
    } else { //else ip is not already in list so add i
        
    }
}



void GttTable::printTheMap(){
    cout<<PURPLE_CODE<<"**********************************"<<"\n";
    cout<<"print the GTT table:"<<"\n";
    for (auto x : m_GttMap) {
        cout << x.first << " ";
        for(auto n : x.second ){
            cout<< n.toUri()<<" "; 
        }
        cout <<"\n";
    }
    cout<<"**********************************"<<"\n"<<END_CODE;
}


