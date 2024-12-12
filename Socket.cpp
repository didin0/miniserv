#include "Socket.hpp"
#include <iostream>
#include <cstring>

Socket::Socket(int domain, int service, int protocol, int port, u_long interface) {
    socketFd = socket(domain, service, protocol);
    if (socketFd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure l'adresse
    address.sin_family = domain;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(interface);

    // Bind
    if (bind(socketFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(socketFd);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(socketFd, 5) < 0) {
        perror("Listen failed");
        close(socketFd);
        exit(EXIT_FAILURE);
    }
}

Socket::~Socket() {
    close(socketFd);
}

int Socket::getSocketFd() const {
    return socketFd;
}
