#include "HttpServer.hpp"
#include "HttpUtils.hpp"

HttpServer::HttpServer(int port) : port(port) {}

void HttpServer::run() {
    setupServerSocket();
    FD_ZERO(&master_read_set);
    FD_SET(server_fd, &master_read_set);
    int max_sd = server_fd;

    while (true) {
        fd_set read_set = master_read_set;
        fd_set write_set = master_write_set;
        struct timeval timeout = {5, 0};

        int activity = select(max_sd + 1, &read_set, &write_set, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            continue;
        }

        // Handle new connections
        if (FD_ISSET(server_fd, &read_set)) {
            handleNewConnections(max_sd);
        }

        // Handle read operations
        std::vector<int> to_remove_read;
        std::vector<int> to_move_to_write;
        processReadOperations(read_set, to_remove_read, to_move_to_write);

        // Handle write operations
        std::vector<int> to_remove_write;
        processWriteOperations(write_set, to_remove_write);

        // Cleanup and update sets
        cleanupConnections(to_remove_read, to_move_to_write, to_remove_write);
        max_sd = updateMaxFd();
    }
}

// Helper methods
void HttpServer::handleNewConnections(int& max_sd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) break;
            std::cerr << "Accept error: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Set non-blocking mode
        fcntl(client_fd, F_SETFL, O_NONBLOCK);
        
        FD_SET(client_fd, &master_read_set);
        read_buffers[client_fd] = "";
        if (client_fd > max_sd) max_sd = client_fd;
    }
}

void HttpServer::processReadOperations(fd_set& read_set, 
                                     std::vector<int>& to_remove_read,
                                     std::vector<int>& to_move_to_write) {
    for (std::map<int, std::string>::iterator it = read_buffers.begin(); it != read_buffers.end(); ++it) {
        int sd = it->first;
        if (FD_ISSET(sd, &read_set)) {
            char buffer[4096];
            ssize_t bytes_read;
            
            while ((bytes_read = recv(sd, buffer, sizeof(buffer), 0)) > 0) {
                it->second.append(buffer, bytes_read);
            }
            
            if (bytes_read == 0 || (bytes_read < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))) {
                to_remove_read.push_back(sd);
            }
            
            if (isRequestComplete(it->second)) {
                handleConnection(sd, it->second);
                to_move_to_write.push_back(sd);
            }
        }
    }
}

void HttpServer::processWriteOperations(fd_set& write_set, std::vector<int>& to_remove_write) {
    for (std::map<int, std::pair<std::string, size_t> >::iterator it = write_buffers.begin(); 
         it != write_buffers.end(); ++it) {
        int sd = it->first;
        if (FD_ISSET(sd, &write_set)) {
            std::string& data = it->second.first;
            size_t& offset = it->second.second;
            
            ssize_t bytes_sent = send(sd, data.c_str() + offset, data.size() - offset, 0);
            if (bytes_sent > 0) {
                offset += bytes_sent;
                if (offset >= data.size()) {
                    to_remove_write.push_back(sd);
                }
            } else if (bytes_sent < 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
                to_remove_write.push_back(sd);
            }
        }
    }
}

void HttpServer::cleanupConnections(std::vector<int>& to_remove_read,
                                  std::vector<int>& to_move_to_write,
                                  std::vector<int>& to_remove_write) {
    // Remove from read set
    for (size_t i = 0; i < to_remove_read.size(); ++i) {
        int sd = to_remove_read[i];
        close(sd);
        FD_CLR(sd, &master_read_set);
        read_buffers.erase(sd);
    }

    // Move to write set
    for (size_t i = 0; i < to_move_to_write.size(); ++i) {
        int sd = to_move_to_write[i];
        FD_CLR(sd, &master_read_set);
        FD_SET(sd, &master_write_set);
        read_buffers.erase(sd);
    }

    // Remove from write set
    for (size_t i = 0; i < to_remove_write.size(); ++i) {
        int sd = to_remove_write[i];
        close(sd);
        FD_CLR(sd, &master_write_set);
        write_buffers.erase(sd);
    }
}

int HttpServer::updateMaxFd() {
    int max = server_fd;
    for (std::map<int, std::string>::iterator it = read_buffers.begin(); it != read_buffers.end(); ++it) {
        if (it->first > max) max = it->first;
    }
    for (std::map<int, std::pair<std::string, size_t> >::iterator it = write_buffers.begin(); 
         it != write_buffers.end(); ++it) {
        if (it->first > max) max = it->first;
    }
    return max;
}

void HttpServer::setupServerSocket() {
    struct sockaddr_in serv_addr = {
        AF_INET,
        htons(port),
        {htonl(INADDR_ANY)},
        {0} // Initialisation explicite de sin_zero
    };


    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }

    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }

    if (listen(server_fd, 5) < 0) {
        throw std::runtime_error("Listen failed: " + std::string(strerror(errno)));
    }

    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
}

void HttpServer::handleConnection(int client_fd, const std::string& request) {
    std::string method, path, httpVersion;
    parseRequestLine(request, method, path, httpVersion);

    std::string response;
    if (method == "GET") {
        response = handleGetResponse(path);
    } else if (method == "POST") {
        response = handlePostResponse(path, request);
    } else {
        response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
    }

    write_buffers[client_fd] = std::make_pair(response, 0);
}

std::string HttpServer::handleGetResponse(const std::string& path) {
    // ... existing handleGet logic to generate response string ...
       std::string filename = path.substr(1); // Retire le '/' initial
    if (filename.empty()) filename = "www/index.html";
    else filename = "www/" + filename;

    std::ifstream file(filename.c_str(), std::ios::binary);

    // Détecter le type MIME
    std::string contentType = "application/octet-stream"; // Par défaut
    size_t extPos = filename.rfind('.');
    if (extPos != std::string::npos) {
        std::string extension = filename.substr(extPos);
        if (extension == ".html" || extension == ".htm") {
            contentType = "text/html";
        } else if (extension == ".css") {
            contentType = "text/css";
        } else if (extension == ".js") {
            contentType = "application/javascript";
        } else if (extension == ".png") {
            contentType = "image/png";
        } else if (extension == ".jpg" || extension == ".jpeg") {
            contentType = "image/jpeg";
        } else if (extension == ".gif") {
            contentType = "image/gif";
        }
    }

    // Lire le contenu du fichier
    std::ostringstream content;
    content << file.rdbuf();
    file.close();

    // Construire la réponse HTTP
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: " << contentType << "\r\n"
            << "Content-Length: " << content.str().size() << "\r\n\r\n"
            << content.str();
    return response.str();
}


std::string HttpServer::handlePostResponse(const std::string& path, const std::string& request) {
    // ... existing handlePost logic to generate response string ...
       (void)path;
    std::string body = extractBody(request);

    // Rechercher la position de "filename="
    size_t filenamePos = request.find("filename=\"");

    // Extraire le nom du fichier
    size_t filenameStart = filenamePos + 10; // Sauter "filename=\""
    size_t filenameEnd = request.find("\"", filenameStart);

    std::string filename = request.substr(filenameStart, filenameEnd - filenameStart);

    std::cout << "Extracted filename: " << filename << std::endl;

    // Sauvegarder le fichier dans le dossier "uploads"
    std::string filePath = "uploads/" + filename;
    std::ofstream file(filePath.c_str(), std::ios::binary);


    // Extraire le contenu après les headers multipart
    size_t fileContentStart = body.find("\r\n\r\n");


    fileContentStart += 4; // Sauter les "\r\n\r\n"
    size_t fileContentEnd = body.rfind("\r\n--");
    if (fileContentEnd == std::string::npos) {
        fileContentEnd = body.size();
    }

    std::string fileContent = body.substr(fileContentStart, fileContentEnd - fileContentStart);
    file << fileContent;
    file.close();

    std::cout << "File saved to: " << filePath << std::endl;

    // Réponse HTTP
    std::ostringstream response;
    response << "HTTP/1.1 201 Created\r\n"
            << "Content-Length: " << fileContent.size() << "\r\n\r\n"
            << "File uploaded successfully: " << filename;
    std::cout << "*************** Response: " << response.str() << std::endl;

    return response.str();
}

void HttpServer::parseRequestLine(const std::string& request, std::string& method, std::string& path, std::string& httpVersion) {
    std::istringstream requestStream(request);
    requestStream >> method >> path >> httpVersion;
        std::cout << "Method: " << method << std::endl;
        std::cout << "Path: " << path << std::endl;
        std::cout << "Version: " << httpVersion << std::endl;
}

void HttpServer::handleGet(int client_fd, const std::string& path) {
    std::string filename = path.substr(1); // Retire le '/' initial
    if (filename.empty()) filename = "www/index.html";
    else filename = "www/" + filename;

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        sendResponse(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    // Détecter le type MIME
    std::string contentType = "application/octet-stream"; // Par défaut
    size_t extPos = filename.rfind('.');
    if (extPos != std::string::npos) {
        std::string extension = filename.substr(extPos);
        if (extension == ".html" || extension == ".htm") {
            contentType = "text/html";
        } else if (extension == ".css") {
            contentType = "text/css";
        } else if (extension == ".js") {
            contentType = "application/javascript";
        } else if (extension == ".png") {
            contentType = "image/png";
        } else if (extension == ".jpg" || extension == ".jpeg") {
            contentType = "image/jpeg";
        } else if (extension == ".gif") {
            contentType = "image/gif";
        }
    }

    // Lire le contenu du fichier
    std::ostringstream content;
    content << file.rdbuf();
    file.close();

    // Construire la réponse HTTP
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: " << contentType << "\r\n"
            << "Content-Length: " << content.str().size() << "\r\n\r\n"
            << content.str();

    sendResponse(client_fd, response.str());
}

void HttpServer::handlePost(int client_fd, const std::string& path, const std::string& request) {
    (void)path;
    std::string body = extractBody(request);

    // Rechercher la position de "filename="
    size_t filenamePos = request.find("filename=\"");
    if (filenamePos == std::string::npos) {
        std::cerr << "Filename not found in request.\n";
        sendResponse(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return;
    }

    // Extraire le nom du fichier
    size_t filenameStart = filenamePos + 10; // Sauter "filename=\""    
    size_t filenameEnd = request.find("\"", filenameStart);
    if (filenameEnd == std::string::npos) {
        std::cerr << "Invalid filename format in request.\n";
        sendResponse(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return;
    }
    std::string filename = request.substr(filenameStart, filenameEnd - filenameStart);

    std::cout << "Extracted filename: " << filename << std::endl;

    // Sauvegarder le fichier dans le dossier "uploads"
    std::string filePath = "uploads/" + filename;
    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        sendResponse(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        return;
    }

    // Extraire le contenu après les headers multipart
    size_t fileContentStart = body.find("\r\n\r\n");
    if (fileContentStart == std::string::npos) {
        std::cerr << "File content not found in request body.\n";
        sendResponse(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return;
    }
    fileContentStart += 4; // Sauter les "\r\n\r\n"
    size_t fileContentEnd = body.rfind("\r\n--");
    if (fileContentEnd == std::string::npos) {
        fileContentEnd = body.size();
    }

    std::string fileContent = body.substr(fileContentStart, fileContentEnd - fileContentStart);
    file << fileContent;
    file.close();

    std::cout << "File saved to: " << filePath << std::endl;

    // Réponse HTTP
    std::ostringstream response;
    response << "HTTP/1.1 201 Created\r\n"
            << "Content-Length: " << fileContent.size() << "\r\n\r\n"
            << "File uploaded successfully: " << filename;
    std::cout << "*************** Response: " << response.str() << std::endl;

    sendResponse(client_fd, response.str());
}

bool HttpServer::isRequestComplete(const std::string& request) {
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false; // Incomplete headers
    }

    // Check for Content-Length for requests with a body
    size_t contentLengthPos = request.find("Content-Length: ");
    if (contentLengthPos != std::string::npos) {
        size_t start = contentLengthPos + 16; // Length of "Content-Length: "
        size_t end = request.find("\r\n", start);
        if (end == std::string::npos) {
            return false; // Invalid Content-Length header
        }

        // Use atoi instead of stoi
        int contentLength = std::atoi(request.substr(start, end - start).c_str());

        size_t bodyStart = headerEnd + 4; // The body starts after "\r\n\r\n"
        return request.size() >= bodyStart + contentLength;
    }

    return true; // No body, probably a request with only headers
}


void HttpServer::sendResponse(int client_fd, const std::string& response) {
    int bytesSent = send(client_fd, response.c_str(), response.size(), 0);
    if (bytesSent < 0) {
        std::cerr << "Failed to send response: " << strerror(errno) << std::endl;
    }
}