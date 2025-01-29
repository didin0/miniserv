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
#include <fcntl.h>

#define PORT 4221

class HttpServer {
public:
    explicit HttpServer(int port);
    void run();

private:
    int port;
    int server_fd;       // Descripteur du socket principal
    std::map<int, std::string> clientBuffers; // Track client buffers
    fd_set master_read_set, master_write_set;
    std::map<int, std::string> read_buffers;
    std::map<int, std::pair<std::string, size_t> > write_buffers;


    void setupServerSocket();
    void handleConnection(int client_fd, const std::string& request);
    void handleGet(int client_fd, const std::string& path);
    void handlePost(int client_fd, const std::string& path, const std::string& request);
    std::string handleGetResponse(const std::string& path);
    std::string handlePostResponse(const std::string& path, const std::string& request);
    void handleNewConnections(int& max_sd);
    void processReadOperations(fd_set& read_set, 
                                     std::vector<int>& to_remove_read,
                                     std::vector<int>& to_move_to_write);
    void processWriteOperations(fd_set& write_set, std::vector<int>& to_remove_write);
    void cleanupConnections(std::vector<int>& to_remove_read,
                                  std::vector<int>& to_move_to_write,
                                  std::vector<int>& to_remove_write);

    bool isRequestComplete(const std::string& request);
    void sendResponse(int client_fd, const std::string& response);
    void parseRequestLine(const std::string& request, std::string& method, std::string& path, std::string& httpVersion);
    void closeConnection(int client_fd);
    int updateMaxFd();
};

#endif // HTTPSERVER_HPP