#ifndef LISTENER_HPP
#define LISTENER_HPP

#include "HttpServer.hpp"
#include <map>
#include <sys/select.h>
#include <string.h>
#include <errno.h>

class Listener {
public:
    void addServer(HttpServer* server);
    void run();

private:
    struct ClientInfo {
        HttpServer* server;
        std::string read_buffer;
        std::string write_buffer;
        size_t write_offset;
    };

    void setupFdSets(fd_set& read_fds, fd_set& write_fds, int& max_fd);
    void handleNewConnections(HttpServer* server);
    void processClientData(int client_fd, ClientInfo& client_info);
    void processClientWrite(int client_fd, ClientInfo& client_info);
    bool isRequestComplete(const std::string& request) const;

    std::map<int, HttpServer*> servers; // Maps server_fd to HttpServer*
    std::map<int, ClientInfo> clients;  // Maps client_fd to ClientInfo
};

#endif