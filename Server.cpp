#include "Server.hpp"
#include "HttpRequests.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>  // Pour perror
#include <unistd.h> // Pour close
#include <fstream>


Server::Server(int port) {
    serverSocket = new Socket(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);

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
    clientBuffers[clientSocket] = "";
    if (clientSocket > maxFd) maxFd = clientSocket;
}

void Server::handleClient(int clientFd) {
    char buffer[4096] = {0};
    int bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
        std::cout << "Connection closed by client: " << clientFd << "\n";
        close(clientFd);
        FD_CLR(clientFd, &masterSet);
        return;
    }

    try {
        HttpRequests request;
        request.parse(buffer);

        std::string response;
        if (request.method == "GET" && request.path == "/") {
            response = generateHttpResponse(200, readHtmlFromFile("www/index.html"));
        } else if (request.method == "GET" && request.path == "/about.html") {
            response = generateHttpResponse(200, readHtmlFromFile("www/about.html"));
        } else {
            response = generateHttpResponse(404, readHtmlFromFile("www/404.html"));
        }

        send(clientFd, response.c_str(), response.size(), 0);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::string errorResponse = generateHttpResponse(400, "Bad Request");
        send(clientFd, errorResponse.c_str(), errorResponse.size(), 0);
    }

    close(clientFd);
    FD_CLR(clientFd, &masterSet);
}


std::string Server::generateHttpResponse(int statusCode, const std::string& content) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << getStatusMessage(statusCode) << "\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "Content-Type: text/html\r\n\r\n";
    response << content;
    return response.str();
}

std::string Server::getStatusMessage(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 404: return "Not Found";
        case 400: return "Bad Request";
        case 405: return "Method Not Allowed";
        default: return "Internal Server Error";
    }
}

std::string Server::readHtmlFromFile(const std::string& filePath) {
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    return content;
}