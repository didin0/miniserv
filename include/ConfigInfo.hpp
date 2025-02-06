#include <iostream>
#include <string>
#include "Listener.hpp"
#include <vector>
#include <unordered_map>

class ConfigInfo {
    public:

    private:
        int nbServer;
        int port;
        std::string serverName;
        std::string host;
        std::string root;
        std::string errorPage;
        int maxBodySize;
        std::unordered_map<std::string, std::vector<std::string>> route; // route[/] = {"GET", "POST"}

};