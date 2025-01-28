#include "HttpServer.hpp"
#include "HttpUtils.hpp"

HttpServer::HttpServer(int port) : port(port) {}

void HttpServer::run() {
    setupServerSocket();
    std::cout << "Server started on port " << port << std::endl;

    fd_set master_set, read_set; // set ecrire et lire 
    int max_sd;
    std::map<int, std::string> client_buffers;

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    max_sd = server_fd;
    int count =0;

    while(true) {
        count++;
        std::cout << "Loop count: " << count << std::endl;
        read_set = master_set;
        struct timeval timeout = {5, 0}; // 5 second timeout

        int activity = select(max_sd + 1, &read_set, NULL, NULL, &timeout);
        if ((activity < 0) && (errno != EINTR)) {
            std::cerr << "Select error: " << strerror(errno) << std::endl;
        }

        // Handle incoming connections
        if (FD_ISSET(server_fd, &read_set)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd < 0) {
                std::cerr << "Accept error: " << strerror(errno) << std::endl;
                continue;
            }

            FD_SET(client_fd, &master_set);
            client_buffers.insert(std::make_pair(client_fd, std::string()));
            if (client_fd > max_sd) max_sd = client_fd;
            std::cout << "New connection: " << client_fd << std::endl;
        }

        // Handle existing clients
        std::vector<int> to_remove;
        for (std::map<int, std::string>::iterator it = client_buffers.begin(); it != client_buffers.end(); ++it) {
            int sd = it->first;
            
            if (FD_ISSET(sd, &read_set)) {
                char buffer[4096];
                ssize_t bytes_read = recv(sd, buffer, sizeof(buffer), 0);

                if (bytes_read <= 0) {
                    // Connection closed
                    close(sd);
                    FD_CLR(sd, &master_set);
                    to_remove.push_back(sd);
                    std::cout << "Connection closed: " << sd << std::endl;
                } else {
                    // Append data to buffer
                    it->second.append(buffer, bytes_read);
                    std::cout << "Received " << bytes_read << " bytes from " << sd << std::endl;

                    // Check if request is complete
                    if (isRequestComplete(it->second)) {
                        handleConnection(sd, it->second);
                        close(sd);
                        FD_CLR(sd, &master_set);
                        to_remove.push_back(sd);
                    }
                }
            }
        }

        // Cleanup closed connections
        for (std::vector<int>::iterator it = to_remove.begin(); it != to_remove.end(); ++it) {
            client_buffers.erase(*it);
        }
    }
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
}

void HttpServer::handleConnection(int client_fd, const std::string& request) {
    std::cout << "Processing request from " << client_fd << std::endl;

    std::string method, path, httpVersion;
    parseRequestLine(request, method, path, httpVersion);

    if (method == "GET") {
        handleGet(client_fd, path);
    } else if (method == "POST") {
        handlePost(client_fd, path, request);
    } else {
        sendResponse(client_fd, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
    }
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