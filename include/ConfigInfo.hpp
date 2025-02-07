#pragma once

#include <iostream>
#include <string>
#include "Listener.hpp"
#include <vector>
#include <unordered_map>
#include "RouteRules.hpp"

class ConfigInfo {
    public:
        ConfigInfo(); 
        ConfigInfo(ConfigInfo const &ref); 
        ConfigInfo& operator=(ConfigInfo const &ref); 
        ~ConfigInfo(); 
    private:
        int nbServer;
        int port;
        std::string serverName;
        std::string host;
        std::string root;
        std::string errorPage;
        int maxBodySize;
        std::map<std::string, RouteRules> RouteMap;
};