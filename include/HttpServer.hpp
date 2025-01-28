#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <utility>
#include <iomanip>
#include "HttpRequest.hpp"
#include <sys/epoll.h>
#include <map>
#include <string>
#include <algorithm>

#define PORT 4221

class HttpServer {
public:
    explicit HttpServer(int port);
    void run();

private:
    int port;
    int server_fd;       // Descripteur du socket principal
    std::map<int, std::string> clientBuffers; // Track client buffers


    void setupServerSocket();
    void handleConnection(int client_fd, const std::string& request);
    void handleGet(int client_fd, const std::string& path);
    void handlePost(int client_fd, const std::string& path, const std::string& request);

    bool isRequestComplete(const std::string& request);
    void sendResponse(int client_fd, const std::string& response);
    void parseRequestLine(const std::string& request, std::string& method, std::string& path, std::string& httpVersion);
    void closeConnection(int client_fd);
};

#endif // HTTPSERVER_HPP