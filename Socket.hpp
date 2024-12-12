#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <cstdlib>

class Socket {
public:
    Socket(int domain, int service, int protocol, int port, u_long interface);
    ~Socket();

    int getSocketFd() const;

private:
    int socketFd;
    sockaddr_in address;
};

#endif
