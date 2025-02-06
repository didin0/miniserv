#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include <string>
#include <stdexcept>
#include <netinet/in.h>
#include <string.h>
#include "ConfigServer.hpp"
#include <vector>

class ConfigServer;
class HttpServer {
public:
    HttpServer(int port);
    ~HttpServer();
    
    int getServerFd() const;
    std::string handleRequest(const std::string& request);

private:
    int server_fd;
    int port;
    struct sockaddr_in serv_addr;
    ConfigServer _mycfg; 

    void setupServerSocket();
    std::string handleGetResponse(const std::string& path);
    std::string handlePostResponse(const std::string& path, const std::string& request);
    void parseRequestLine(const std::string& request, 
                         std::string& method, 
                         std::string& path, 
                         std::string& httpVersion);
    bool isRequestComplete(const std::string& request) const;
    std::string extractBody(const std::string& request) const;

    //// ROUTE / METHODE 
     void set_route(std::vector<std::string > v );
     void set_methode(std::vector<std::string > v );
     std::vector<std::string> get_route() const;
     std::vector<std::string> get_methode() const;

      void display_vector(std::vector<std::string> v) const ; // afficher route / methode  
      bool check_config(std::string s) ; // check the config for  each request 

};

#endif