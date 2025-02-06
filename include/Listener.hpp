#ifndef LISTENER_HPP
#define LISTENER_HPP

#include "HttpServer.hpp"
#include <map>
#include <sys/select.h>
#include <string.h>
#include <errno.h>
#include "ClientInfo.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <fstream>
class ClientInfo;

class Listener {
public:
    void addServer(HttpServer* server);
    void run();
    std::string av;
    void initConf(char **argv);


private:
    void setupFdSets(fd_set& read_fds, fd_set& write_fds, int& max_fd);
    void handleNewConnections(HttpServer* server);
    void processClientData(int client_fd, ClientInfo& client_info);
    void processClientWrite(int client_fd, ClientInfo& client_info);
    bool isRequestComplete(const std::string& request) const;

    std::map<int, HttpServer*> servers; // Maps server_fd to HttpServer*
    std::map<int, ClientInfo> clients;  // Maps client_fd to ClientInfo
};

#endif