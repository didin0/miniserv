#define _GLIBCXX_USE_C99 1
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <utility>
#include <iomanip>

#define PORT 4221

class HttpServer {
public:
    HttpServer(int port) : port(port) {}

    void run() {
        setupServerSocket();
        std::cout << "Server started on port " << port << std::endl;

        while (true) {
            std::cout << "Waiting for clients to connect..." << std::endl;

            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

            if (client_fd < 0) {
                std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
                continue;
            }

            std::cout << "Client connected." << std::endl;

            if (!fork()) { // Child process
                close(server_fd);
                handleConnection(client_fd);
                close(client_fd);
                exit(0);
            }

            close(client_fd); // Parent process closes the client socket
        }
    }

private:
    int port;
    int server_fd;

    void setupServerSocket() {
        struct sockaddr_in serv_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {htonl(INADDR_ANY)}
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

    void handleConnection(int client_fd) {
        std::string request;
        char buffer[4096]; // Taille du tampon fixe

        while (true) {
            int bytesReceived = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytesReceived < 0) {
                std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
                return;
            }
            if (bytesReceived == 0) break; // Le client a fermé la connexion

            request.append(buffer, bytesReceived);

            // Vérifiez si toute la requête est reçue
            if (isRequestComplete(request)) {
                break;
            }
        }

        std::cout << "Full request received:\n" << request << std::endl;

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

    bool isRequestComplete(const std::string& request) {
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            return false; // En-têtes incomplets
        }

        // Vérifiez la présence de Content-Length pour les requêtes avec un corps
        size_t contentLengthPos = request.find("Content-Length: ");
        if (contentLengthPos != std::string::npos) {
            size_t start = contentLengthPos + 16; // Longueur de "Content-Length: "
            size_t end = request.find("\r\n", start);
            int contentLength = std::stoi(request.substr(start, end - start));

            size_t bodyStart = headerEnd + 4; // Le corps commence après "\r\n\r\n"
            return request.size() >= bodyStart + contentLength;
        }

        return true; // Aucune longueur spécifiée, probablement une requête sans corps
    }



    void parseRequestLine(const std::string& request, std::string& method, std::string& path, std::string& httpVersion) {
        std::istringstream requestStream(request);
        requestStream >> method >> path >> httpVersion;
    }
        
    void handleGet(int client_fd, const std::string& path) {
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


    void handlePost(int client_fd, const std::string& path, const std::string& request) {
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

        sendResponse(client_fd, response.str());
    }


    std::string extractBody(const std::string& request) {
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            return request.substr(bodyPos + 4);
        }
        return "";
    }

    void sendResponse(int client_fd, const std::string& response) {
        int bytesSent = send(client_fd, response.c_str(), response.size(), 0);
        if (bytesSent < 0) {
            std::cerr << "Failed to send response: " << strerror(errno) << std::endl;
        }
    }
};

int main() {
    try {
        HttpServer server(PORT);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
