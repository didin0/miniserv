#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <set>
#include <sys/select.h>
#include "Socket.hpp"
#include "HttpRequests.hpp"

class Server {
public:
    Server(int port);
    ~Server();
    void run();

private:
    Socket* serverSocket;
    fd_set masterSet, readSet;
    int maxFd;
    std::map<int, std::string> clientBuffers;

    void handleNewConnection();
    void handleClient(int clientFd);

    std::string generateHttpResponse(int statusCode, const std::string& content);
    std::string getStatusMessage(int statusCode);
    std::string readHtmlFromFile(const std::string& filePath);

};

#endif
