#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include "HttpRequests.hpp"
#include "RouteHandler.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <sys/select.h>

class Server {
private:
    Socket* serverSocket;
    fd_set masterSet, readSet;
    int maxFd;
    std::map<int, std::string> clientBuffers;
    RouteHandler routeHandler; // Nouveau membre pour gérer les autorisations

    void configureRoutes(); // Configuration des routes autorisées

    void handleNewConnection();
    void handleClient(int clientFd);
    void handleGet(const HttpRequests& request, int clientFd);
    void handlePost(const HttpRequests& request, int clientFd);
    void handleDelete(const HttpRequests& request, int clientFd);
    std::string generateHttpResponse(int statusCode, const std::string& content);
    std::string getStatusMessage(int statusCode);
    std::string readHtmlFromFile(const std::string& filePath);
    bool isRequestComplete(const std::string& request);


public:
    Server(int port);
    ~Server();
    void run();
};

#endif
