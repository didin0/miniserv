#pragma once

#include <iostream>
#include <string>
#include "Listener.hpp"
#include <vector>
#include <unordered_map>

class RouteRules {
    public:

    private:
        std::vector<std::string> acceptedMethods;
        std::string redirection;
        std::string rootDirectory;
        bool autoIndex;
        std::string defaultFile;
        bool allowUploads;
        std::string uploadDirectory;
};