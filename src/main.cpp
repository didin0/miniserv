#include "HttpServer.hpp"
#include <iostream>

int main() {
    try {
        HttpServer server(4221);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}