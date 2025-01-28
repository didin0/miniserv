#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h> // Pour mkdir
#include <sstream>    // Pour std::ostringstream
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

#define PORT 8080

void ensureUploadsDirectoryExists() {
    if (access("uploads", F_OK) != 0) {
        if (mkdir("uploads", 0777) != 0) {
            perror("Failed to create uploads directory");
        } else {
            std::cout << "Uploads directory created.\n";
        }
    }
}

std::ostringstream generateHttpResponse(int statusCode, const std::string& content) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " OK\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "Content-Type: text/html\r\n\r\n";
    response << content;
    return response;
}

void handleMultipartRequest(const std::string& body, const std::string& boundary) {
    std::string delimiter = boundary + "\r\n"; // Chaque partie est précédée par le délimiteur avec "\r\n"
    size_t partStart = body.find(delimiter);

    if (partStart == std::string::npos) {
        std::cerr << "Boundary not found in the body.\n";
        return;
    }

    partStart += delimiter.size();

    while (partStart < body.size()) {
        size_t partEnd = body.find(boundary, partStart);
        if (partEnd == std::string::npos) break;

        std::string part = body.substr(partStart, partEnd - partStart);
        partStart = partEnd + boundary.size();

        // Vérifiez que la partie contient des en-têtes multipart
        if (part.find("Content-Disposition") == std::string::npos) continue;

        // Extraire le nom de fichier
        size_t fileNamePos = part.find("filename=\"");
        if (fileNamePos != std::string::npos) {
            size_t fileNameEnd = part.find("\"", fileNamePos + 10);
            std::string fileName = part.substr(fileNamePos + 10, fileNameEnd - fileNamePos - 10);

            // Extraire le contenu du fichier
            size_t fileContentStart = part.find("\r\n\r\n", fileNameEnd) + 4;
            size_t fileContentEnd = part.rfind("\r\n");
            if (fileContentStart == std::string::npos || fileContentEnd == std::string::npos || fileContentEnd < fileContentStart) {
                std::cerr << "Invalid file content structure.\n";
                continue;
            }

            std::string fileContent = part.substr(fileContentStart, fileContentEnd - fileContentStart);

            std::cout << "Filename: " << fileName << "\n";
            std::cout << "File content:\n" << fileContent << "\n";

            // Sauvegarder le fichier
            std::ofstream outFile("uploads/" + fileName, std::ios::binary);
            if (outFile.is_open()) {
                outFile << fileContent;
                outFile.close();
                std::cout << "File saved to: uploads/" << fileName << "\n";
            } else {
                std::cerr << "Failed to save file: " << fileName << "\n";
            }
        }
    }
}


void handleRequest(int clientSocket) {
    char buffer[8192];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        perror("Failed to read from client");
        close(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer);
    std::stringstream   ss(buffer);


    std::cout << "Full request:\n" << ss.str() << "\n";

    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos == std::string::npos) {
        std::cerr << "Invalid HTTP request: No body found.\n";
        close(clientSocket);
        return;
    }

    std::string body = request.substr(bodyPos + 4);

    if (request.find("Content-Type: multipart/form-data") != std::string::npos) {
        size_t boundaryPos = request.find("boundary=");
        if (boundaryPos == std::string::npos) {
            std::cerr << "Invalid multipart request: No boundary found.\n";
            close(clientSocket);
            return;
        }

        std::string boundary = "--" + request.substr(boundaryPos + 9);
        handleMultipartRequest(body, boundary);

        std::ostringstream response = generateHttpResponse(200, "<html><body>File uploaded successfully!</body></html>");    
        send(clientSocket, response.str(), response, 0);
    } else {
        std::cerr << "Request is not multipart. No file uploaded.\n";
    }
    bzero(buffer, sizeof(buffer));
    close(clientSocket);
}

int main() {
    ensureUploadsDirectoryExists();

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int reuse = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
	{
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
	}

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    std::cout << "Server is running on http://localhost:" << PORT << "\n";

    while (true) {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }
        handleRequest(clientSocket);
    }

    close(serverSocket);
    return 0;
}
