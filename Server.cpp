#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>

Server::Server(int port) {
    serverSocket = new Socket(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);

    // Initialiser fd_set
    FD_ZERO(&masterSet);
    FD_SET(serverSocket->getSocketFd(), &masterSet);
    maxFd = serverSocket->getSocketFd();
}

Server::~Server() {
    delete serverSocket;
}

void Server::run() {
    while (true) {
        readSet = masterSet;

        int activity = select(maxFd + 1, &readSet, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            break;
        }

        for (int fd = 0; fd <= maxFd; ++fd) {
            if (FD_ISSET(fd, &readSet)) {
                if (fd == serverSocket->getSocketFd()) {
                    handleNewConnection();
                } else {
                    handleClient(fd);
                }
            }
        }
    }
}

void Server::handleNewConnection() {
    int clientSocket = accept(serverSocket->getSocketFd(), NULL, NULL);
    if (clientSocket < 0) {
        perror("Accept failed");
        return;
    }
    FD_SET(clientSocket, &masterSet);
    clientBuffers[clientSocket] = ""; // Initialiser le tampon pour ce client
    if (clientSocket > maxFd) maxFd = clientSocket;

}

void Server::handleClient(int clientFd) {
    char buffer[1024] = {0};
    int bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0) {
        // Client a fermé la connexion
        std::cout << "Connection closed by client: " << clientFd << "\n";
        close(clientFd);
        FD_CLR(clientFd, &masterSet);
        clientBuffers.erase(clientFd); // Supprimer le tampon du client
    } else {
        // Ajouter les données reçues au tampon du client
        clientBuffers[clientFd] += std::string(buffer, bytesRead);

        // Vérifier si la requête est complète (fini par "\r\n\r\n")
        if (clientBuffers[clientFd].find("\r\n\r\n") != std::string::npos) {
            // Traiter la requête complète
            processRequest(clientFd, clientBuffers[clientFd]);

            // Supprimer le tampon une fois traité
            clientBuffers.erase(clientFd);
        }
    }
}

void Server::processRequest(int clientFd, const std::string& request) {
    std::cout << "\033[33mNew connection, socket fd: " << clientFd << "\033[0m\n";
    std::cout << "Request:\n" << request << "\n";

    const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    send(clientFd, response, strlen(response), 0);
    close(clientFd); // Fermer la connexion après la réponse
    FD_CLR(clientFd, &masterSet);
}
