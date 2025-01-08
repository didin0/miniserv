#include "MultipartParser.hpp"
#include <sstream>

std::vector<MultipartParser::Part> MultipartParser::parse(const std::string& body, const std::string& boundary) {
    std::vector<Part> parts;
    std::string delimiter = "--" + boundary;
    size_t start = 0, end;

    while ((end = body.find(delimiter, start)) != std::string::npos) {
        std::string part = body.substr(start, end - start);
        size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            Part p;
            p.headers = part.substr(0, headerEnd);
            p.body = part.substr(headerEnd + 4);
            parts.push_back(p);
        }
        start = end + delimiter.size();
    }

    return parts;
}
