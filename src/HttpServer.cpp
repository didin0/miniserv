#include "HttpServer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>
#include "HttpUtils.hpp"


HttpServer::HttpServer(int port) : port(port) {
    setupServerSocket();
}

HttpServer::~HttpServer() {
    close(server_fd);
}

int HttpServer::getServerFd() const {
    return server_fd;
}

void HttpServer::setupServerSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        throw std::runtime_error("Setsockopt failed: " + std::string(strerror(errno)));
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(server_fd);
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        throw std::runtime_error("Listen failed: " + std::string(strerror(errno)));
    }

    // Set non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
}

std::string HttpServer::handleRequest(const std::string& request) {
    std::string method, path, httpVersion;
    parseRequestLine(request, method, path, httpVersion);

    if (method == "GET") {
        return handleGetResponse(path);
    } else if (method == "POST") {
        return handlePostResponse(path, request);
    }
    return "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
}

std::string HttpServer::handleGetResponse(const std::string& path) {
    std::string filename = path.substr(1);
    if (filename.empty()) filename = "www/index.html";
    else filename = "www/" + filename;

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file) {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }

    // Detect MIME type
    std::string contentType = getMimeType(filename);

    std::ostringstream content;
    content << file.rdbuf();
    file.close();

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << content.str().size() << "\r\n\r\n"
             << content.str();
    return response.str();
}

std::string HttpServer::handlePostResponse(const std::string& path, const std::string& request) {
    (void)path;
    std::string body = extractBody(request);
    size_t filenameStart = request.find("filename=\"") + 10;
    size_t filenameEnd = request.find("\"", filenameStart);
    
    if (filenameStart == std::string::npos || filenameEnd == std::string::npos) {
        return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }

    std::string filename = request.substr(filenameStart, filenameEnd - filenameStart);
    std::string filePath = "uploads/" + filename;

    size_t contentStart = body.find("\r\n\r\n") + 4;
    size_t contentEnd = body.rfind("\r\n--");
    if (contentEnd == std::string::npos) contentEnd = body.size();

    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file) {
        return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    }

    file << body.substr(contentStart, contentEnd - contentStart);
    file.close();

    std::ostringstream response;
    response << "HTTP/1.1 201 Created\r\n"
             << "Content-Length: " << (contentEnd - contentStart) << "\r\n\r\n"
             << "File uploaded successfully: " << filename;

    std::cout << "File uploaded: " << filename << std::endl;
    return response.str();
}

void HttpServer::parseRequestLine(const std::string& request,
                                std::string& method,
                                std::string& path,
                                std::string& httpVersion) {
    std::istringstream requestStream(request);
    requestStream >> method >> path >> httpVersion;
}

std::string HttpServer::extractBody(const std::string& request) const {
    size_t bodyPos = request.find("\r\n\r\n");
    return (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";
}