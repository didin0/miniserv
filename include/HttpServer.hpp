#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include <string>
#include <stdexcept>
#include <netinet/in.h>
#include <string.h>

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

    void setupServerSocket();
    std::string handleGetResponse(const std::string& path);
    std::string handlePostResponse(const std::string& path, const std::string& request);
    void parseRequestLine(const std::string& request, 
                         std::string& method, 
                         std::string& path, 
                         std::string& httpVersion);
    bool isRequestComplete(const std::string& request) const;
    std::string extractBody(const std::string& request) const;
};

#endif