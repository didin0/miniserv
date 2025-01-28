#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <iostream>

class HttpRequest {
public:
    // Attributs publics pour simplifier l'accès
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    // Méthodes
    void parse(const std::string& rawRequest);
    void parseMultipartBody(const std::string& boundary);
    std::string getHeader(const std::string& key) const;
};

#endif
