#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    std::srand(std::time(0));
    int PORT = 8080 + (std::rand() % 6); // random PORT to avoid bind() error

    std::cout << "\033[31mServer is running on port " << PORT << "...\n";
    std::cout << "URL : \033]8;;http://localhost:" << PORT << "\033\\\033[36mhttp://localhost:" << PORT << "\033]8;;\033[0m\n" << std::endl;

    Server server(PORT);
    server.run();

    return 0;
}
