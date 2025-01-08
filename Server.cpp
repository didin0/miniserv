#include "Server.hpp"
#include "HttpRequests.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <fstream>
#include <string>
#include <stdexcept>
#include "RouteHandler.hpp"
#include "MultipartParser.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

Server::Server(int port) {
    serverSocket = new Socket(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY);

    FD_ZERO(&masterSet);
    FD_SET(serverSocket->getSocketFd(), &masterSet);
    maxFd = serverSocket->getSocketFd();

    configureRoutes(); // Configure les autorisations des routes/méthodes
}

void Server::configureRoutes() {
    // Autorise uniquement GET et POST sur `/`
    std::vector<std::string> rootMethods;
    rootMethods.push_back("GET");
    rootMethods.push_back("POST");
    //rootMethods.push_back("DELETE");
    routeHandler.addRoute("/", rootMethods);

    // Autorise uniquement GET sur `/about`
    std::vector<std::string> aboutMethods;
    aboutMethods.push_back("GET");
    aboutMethods.push_back("POST");
    //aboutMethods.push_back("DELETE");
    routeHandler.addRoute("/about", aboutMethods);

    // Interdit DELETE sur `/404`
    std::vector<std::string> notFoundMethods;
    notFoundMethods.push_back("GET");
    notFoundMethods.push_back("POST");
    routeHandler.addRoute("/404", notFoundMethods);
    
    // Autoriser GET et POST sur "/uploads"
    std::vector<std::string> uploadMethods;
    uploadMethods.push_back("POST");
    routeHandler.addRoute("/uploads", uploadMethods);
    
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
        // Le client a fermé la connexion
        close(clientFd);
        FD_CLR(clientFd, &masterSet);
    } else {
        clientBuffers[clientFd] += std::string(buffer, bytesRead); // Ajoute au buffer

        if (isRequestComplete(clientBuffers[clientFd])) {
            try {
                HttpRequests request;
                request.parse(clientBuffers[clientFd]); // Parse la requête complète
                clientBuffers[clientFd].clear(); // Nettoie le buffer après traitement

                // Vérifie si la méthode est autorisée
                if (!routeHandler.isMethodAllowed(request.path, request.method)) {
                    std::string response = generateHttpResponse(405, "Method Not Allowed");
                    send(clientFd, response.c_str(), response.size(), 0);
                } else if (request.method == "GET") {
                    handleGet(request, clientFd);
                } else if (request.method == "POST") {
                    handlePost(request, clientFd);
                } else if (request.method == "DELETE") {
                    handleDelete(request, clientFd);
                } else {
                    std::string response = generateHttpResponse(405, "Method Not Allowed");
                    send(clientFd, response.c_str(), response.size(), 0);
                }

            } catch (const std::exception& e) {
                std::string errorResponse = generateHttpResponse(400, "Bad Request");
                send(clientFd, errorResponse.c_str(), errorResponse.size(), 0);
            }

            close(clientFd);
            FD_CLR(clientFd, &masterSet);
        }
    }
}


void Server::handleGet(const HttpRequests& request, int clientFd) {
    std::string filePath;

    // Si le path est "/", on retourne le fichier index.html
    if (request.path == "/") {
        filePath = "www/index.html";
    } else {
        // Sinon, construisez le chemin avec ajout de ".html" si nécessaire
        filePath = "www" + request.path;
        if (filePath.find('.') == std::string::npos) {
            filePath += ".html";
        }
    }

    std::ifstream file(filePath.c_str());
    std::string response;

    std::cout << "Requested file path: " << filePath << std::endl;

    if (file.is_open()) {
        std::stringstream content;
        content << file.rdbuf();
        file.close();

        response = generateHttpResponse(200, content.str());
    } else {
        response = generateHttpResponse(404, "Not Found");
    }

    send(clientFd, response.c_str(), response.size(), 0);
}

void Server::handlePost(const HttpRequests& request, int clientFd) {
    if (request.headers.find("Content-Type") != request.headers.end()) {
        std::string contentType = request.headers.find("Content-Type")->second;
        if (contentType.find("multipart/form-data") != std::string::npos) {
            size_t boundaryPos = contentType.find("boundary=");
            if (boundaryPos != std::string::npos) {
                std::string boundary = contentType.substr(boundaryPos + 9);

                MultipartParser parser;
                std::vector<MultipartParser::Part> parts = parser.parse(request.body, boundary);

                for (size_t i = 0; i < parts.size(); ++i) {
                    std::cout << "Part headers: " << parts[i].headers << "\n";
                    std::cout << "Part body: " << parts[i].body << "\n";

                    // Utilisation de ostringstream au lieu de std::to_string
                    std::ostringstream oss;
                    oss << "uploads/part_" << i << ".txt";
                    std::ofstream outFile(oss.str().c_str(), std::ios::app);

                    if (outFile.is_open()) {
                        outFile << parts[i].body;
                        outFile.close();
                    }
                }

                std::string response = generateHttpResponse(200, "Multipart Data Processed");
                send(clientFd, response.c_str(), response.size(), 0);
                return;
            }
        }
    }

    // Comportement par défaut si non multipart
    std::ofstream outFile("uploads/data.txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << request.body;
        outFile.close();

        std::string response = generateHttpResponse(200, "Data saved successfully");
        send(clientFd, response.c_str(), response.size(), 0);
    } else {
        std::string response = generateHttpResponse(500, "Internal Server Error");
        send(clientFd, response.c_str(), response.size(), 0);
    }
}


void Server::handleDelete(const HttpRequests& request, int clientFd) {
    std::string filePath = "www" + request.path; // Dossier racine `www`
    if (remove(filePath.c_str()) == 0) {
        std::string response = generateHttpResponse(200, "File deleted successfully");
        send(clientFd, response.c_str(), response.size(), 0);
    } else {
        std::string response = generateHttpResponse(404, "File not found");
        send(clientFd, response.c_str(), response.size(), 0);
    }
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

bool Server::isRequestComplete(const std::string& request) {
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false; // Headers incomplets
    }

    size_t contentLengthPos = request.find("Content-Length: ");
    if (contentLengthPos != std::string::npos) {
        size_t start = contentLengthPos + 16; // Longueur de "Content-Length: "
        size_t end = request.find("\r\n", start);
        int contentLength = atoi(request.substr(start, end - start).c_str());

        // Vérifiez si le corps complet est reçu
        size_t bodyStart = headerEnd + 4;
        return request.size() >= bodyStart + contentLength;
    }

    return true; // Pas de corps, uniquement les headers
}
