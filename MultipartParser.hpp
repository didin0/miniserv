#ifndef MULTIPARTPARSER_HPP
#define MULTIPARTPARSER_HPP

#include <string>
#include <vector>

class MultipartParser {
public:
    struct Part {
        std::string headers;
        std::string body;
    };

    std::vector<Part> parse(const std::string& body, const std::string& boundary);
};

#endif
