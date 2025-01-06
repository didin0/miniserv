#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <netinet/in.h>
#include <unistd.h>

class Socket {
private:
    int socketFd;
    struct sockaddr_in address;

public:
    Socket(int domain, int service, int protocol, int port, u_long interface);
    ~Socket();

    int getSocketFd() const;
};

#endif
