#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include <set>
#include <map>
#include <string>

/*
            server ->  list client
            chemin = "/" -> vector Route 
            metode = GET/POST -> vector Methode 
            client  -> vector -> rajouter chaque client qpres lq methode accept
            _socket;    



*/

class Server {
public:
    Server(int port);
    ~Server();

    void run();

private:
    Socket* serverSocket;
    fd_set masterSet;
    fd_set readSet;
    int maxFd;

    // Stockage temporaire des requêtes clients
    std::map<int, std::string> clientBuffers;

    void handleNewConnection();
    void handleClient(int clientFd);
    void processRequest(int clientFd, const std::string& request);
};

#endif

