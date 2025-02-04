#include "Listener.hpp"
#include "HttpServer.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <vector>

void Listener::addServer(HttpServer* server) {
    int server_fd = server->getServerFd();
    servers[server_fd] = server;
}

void Listener::run() {
    fd_set read_fds, write_fds;
    int max_fd = 0;

    while (true) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        max_fd = 0;

        // Iterate using explicit iterators
        for (std::map<int, HttpServer*>::iterator it = servers.begin(); it != servers.end(); ++it) {
            int fd = it->first;
            FD_SET(fd, &read_fds);
            if (fd > max_fd) max_fd = fd;
        }

        // Client iteration
        for (std::map<int, ClientInfo>::iterator it = clients.begin(); it != clients.end(); ++it) {
            int fd = it->first;
            ClientInfo& info = it->second;

            if (info.write_buffer.empty() || info.write_offset < info.write_buffer.size()) {
                FD_SET(fd, &read_fds);
            }
            if (!info.write_buffer.empty() && info.write_offset < info.write_buffer.size()) {
                FD_SET(fd, &write_fds);
            }
            if (fd > max_fd) max_fd = fd;
        }

        struct timeval timeout = {5, 0};
        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, &timeout);

        if (activity < 0) {
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            continue;
        }

        // Handle server connections
        for (std::map<int, HttpServer*>::iterator it = servers.begin(); it != servers.end(); ++it) {
            if (FD_ISSET(it->first, &read_fds)) {
                handleNewConnections(it->second);
            }
        }

        // Handle client I/O
        std::vector<int> to_remove;
        for (std::map<int, ClientInfo>::iterator it = clients.begin(); it != clients.end(); ++it) {
            int client_fd = it->first;
            ClientInfo& info = it->second;

            if (FD_ISSET(client_fd, &read_fds)) {
                processClientData(client_fd, info);
            }
            if (FD_ISSET(client_fd, &write_fds)) {
                processClientWrite(client_fd, info);
                if (info.write_offset >= info.write_buffer.size()) {
                    to_remove.push_back(client_fd);
                }
            }
        }

        // Cleanup
        for (std::vector<int>::iterator it = to_remove.begin(); it != to_remove.end(); ++it) {
            std::cout << "Closing client " << *it << std::endl;
            close(*it);
            clients.erase(*it);
        }
    }
}

void Listener::handleNewConnections(HttpServer* server) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server->getServerFd(), (struct sockaddr*)&client_addr, &client_len);
    std::cout << "New connection on server " << server->getServerFd() << " from client " << client_fd << std::endl;

    if (client_fd < 0) {
        std::cerr << "Accept error: " << strerror(errno) << std::endl;
        return;
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    
    ClientInfo info;
    info.server = server;
    info.read_buffer = "";
    info.write_buffer = "";
    info.write_offset = 0;
    clients[client_fd] = info;
}

void Listener::processClientData(int client_fd, ClientInfo& client_info) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes_read > 0) {
        client_info.read_buffer.append(buffer, bytes_read);
        if (isRequestComplete(client_info.read_buffer)) {
            std::string response = client_info.server->handleRequest(client_info.read_buffer);
            client_info.write_buffer = response;
            client_info.read_buffer.clear();
        }
    } else if (bytes_read <= 0) {
        close(client_fd);
        clients.erase(client_fd);
    }
}

void Listener::processClientWrite(int client_fd, ClientInfo& client_info) {
    size_t& offset = client_info.write_offset;
    const std::string& buffer = client_info.write_buffer;

    if (offset >= buffer.size()) return;

    ssize_t bytes_sent = send(client_fd, buffer.c_str() + offset, buffer.size() - offset, 0);
    if (bytes_sent > 0) {
        offset += bytes_sent;
    } else if (bytes_sent < 0) {
        close(client_fd);
        clients.erase(client_fd);
    }
}

bool Listener::isRequestComplete(const std::string& request) const {
    size_t header_end = request.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;

    size_t cl_pos = request.find("Content-Length: ");
    if (cl_pos != std::string::npos) {
        size_t start = cl_pos + 16;
        size_t end = request.find("\r\n", start);
        int content_length = std::atoi(request.substr(start, end - start).c_str());
        return request.size() >= header_end + 4 + content_length;
    }

    return true;
}