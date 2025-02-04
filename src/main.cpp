#include <iostream>
#include "Listener.hpp"

#define PORT 8080
#define PORT2 4221

int main() {
    try {
        Listener listener;
        
        HttpServer server1(PORT);
        HttpServer server2(PORT2);
        
        listener.addServer(&server1);
        listener.addServer(&server2);
        
        std::cout << "Listening on port " << PORT << " and " << PORT2 << std::endl;

        listener.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}