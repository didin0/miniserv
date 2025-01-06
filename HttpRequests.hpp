#ifndef HTTPREQUESTS_HPP
#define HTTPREQUESTS_HPP

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <stdexcept>

class HttpRequests {
public:
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    // Analyse une requête HTTP brute
    void parse(const std::string& request);
};

#endif // HTTPREQUESTS_HPP
