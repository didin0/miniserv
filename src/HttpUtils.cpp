#include "HttpUtils.hpp"

std::string getMimeType(const std::string& filename) {
    size_t extPos = filename.rfind('.');
    if (extPos != std::string::npos) {
        std::string extension = filename.substr(extPos);
        if (extension == ".html" || extension == ".htm") {
            return "text/html";
        } else if (extension == ".css") {
            return "text/css";
        } else if (extension == ".js") {
            return "application/javascript";
        } else if (extension == ".png") {
            return "image/png";
        } else if (extension == ".jpg" || extension == ".jpeg") {
            return "image/jpeg";
        } else if (extension == ".gif") {
            return "image/gif";
        }
    }
    return "application/octet-stream";
}

std::string extractBody(const std::string& request) {
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        return request.substr(bodyPos + 4);
    }
    return "";
}