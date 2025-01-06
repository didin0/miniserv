#include "Socket.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <cstdio>

Socket::Socket(int domain, int service, int protocol, int port, u_long interface) {
    socketFd = socket(domain, service, protocol);
    if (socketFd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = domain;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(interface);

    if (bind(socketFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(socketFd);
        exit(EXIT_FAILURE);
    }

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
