#include "HttpRequests.hpp"
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstdio>

void HttpRequests::parse(const std::string& request) {
    std::istringstream stream(request);
    std::string line;

    if (!std::getline(stream, line))
        throw std::runtime_error("Missing request line");

    std::istringstream request_line(line);
    if (!(request_line >> method >> path >> version))
        throw std::runtime_error("Invalid request line");

    while (std::getline(stream, line) && line != "\r") {
        size_t pos = line.find(": ");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);
            headers[key] = value;
        }
    }

    if (headers.find("Content-Length") != headers.end()) {
        std::istringstream iss(headers["Content-Length"]);
        int content_length;
        iss >> content_length;
        body.resize(content_length);
        stream.read(&body[0], content_length);
    }
}
