#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#define PORT 8080

class MiniServer {
public:
    MiniServer();
    ~MiniServer();
    void startServer();
    
private:
    int server_fd;
    struct sockaddr_in address;
    
    void handleClient(int client_socket);
    void handleRequest(int client_socket);
};


std::string generateHTML() {
    std::string html = 
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>File Upload</title>\n"
        "</head>\n"
        "<body>\n"
        "\n"
        "    <h2>Upload a File</h2>\n"
        "    \n"
        "    <form action=\"/upload\" method=\"POST\" enctype=\"multipart/form-data\">\n"
        "        <label for=\"file-upload\">Choose a file to upload:</label>\n"
        "        <input type=\"file\" id=\"file-upload\" name=\"file-upload\" accept=\"*/*\" required>\n"
        "        <br><br>\n"
        "        <button type=\"submit\">Upload File</button>\n"
        "    </form>\n"
        "\n"
        "</body>\n"
        "</html>\n";
    return html;
}

MiniServer::MiniServer() {
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
	{
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
	}

    // Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
}

MiniServer::~MiniServer() {
    close(server_fd);
}

void MiniServer::handleClient(int client_socket) {
    std::cout << "Handling client connection..." << std::endl;
    handleRequest(client_socket);
}

void MiniServer::handleRequest(int client_socket) {
    const char *message = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\nHello, World!";
    send(client_socket, message, strlen(message), 0);
    std::cout << "Response sent to client." << std::endl;
    char buffer[8192];
    int bytesRead = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        perror("Failed to read from client");
        close(client_socket);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer);
    std::cout << request << std::endl;
    close(client_socket);
}

void MiniServer::startServer() {
    int client_socket;
    int addr_len = sizeof(address);

    while (true) {
        std::cout << "Waiting for client connection..." << std::endl;
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        handleClient(client_socket);
    }
}

int main() {
    MiniServer server;
    server.startServer();
    return 0;
}



