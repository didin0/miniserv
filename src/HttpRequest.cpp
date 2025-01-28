#include "HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

void HttpRequest::parse(const std::string& rawRequest) {
    std::istringstream requestStream(rawRequest);
    std::string line;

    // Lire la première ligne (méthode, chemin, version)
    if (std::getline(requestStream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path >> version;
    }

    // Lire les en-têtes
    while (std::getline(requestStream, line) && line != "\r") {
        size_t delimiter = line.find(':');
        if (delimiter != std::string::npos) {
            std::string key = line.substr(0, delimiter);
            std::string value = line.substr(delimiter + 1);
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            headers[key] = value;
        }
    }

    // Lire le corps
    body.assign(std::istreambuf_iterator<char>(requestStream),
                std::istreambuf_iterator<char>());

    // Détecter les requêtes multipart
    if (headers.find("Content-Type") != headers.end()) {
        const std::string& contentType = headers["Content-Type"];
        if (contentType.find("multipart/form-data") != std::string::npos) {
            size_t boundaryPos = contentType.find("boundary=");
            if (boundaryPos != std::string::npos) {
                std::string boundary = "--" + contentType.substr(boundaryPos + 9);
                
                parseMultipartBody(boundary);
            }
        }
    }
}

void HttpRequest::parseMultipartBody(const std::string& boundary) {

    std::cout << "Parsing multipart body with boundary: " << boundary << std::endl;

    size_t partStart = body.find(boundary);
    while (partStart != std::string::npos) {
        size_t partEnd = body.find(boundary, partStart + boundary.size());
        if (partEnd == std::string::npos) break;
            std::cout << " -------------HERE------------"<< std::endl;
        std::string part = body.substr(partStart + boundary.size(), partEnd - partStart - boundary.size());
        partStart = partEnd;

        size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::string headersSection = part.substr(0, headerEnd);
            std::string content = part.substr(headerEnd + 4, part.size() - headerEnd - 6); // Retirer "\r\n--"
                  std::cout << "****************************************" << std::endl;
              std::cout << "content : " << content << std::endl;
            // Extraire le filename
            size_t filenamePos = headersSection.find("filename=\"");
            if (filenamePos != std::string::npos) {
                size_t filenameEnd = headersSection.find("\"", filenamePos + 10);
                std::string filename = headersSection.substr(filenamePos + 10, filenameEnd - filenamePos - 10);

                // Ajouter le fichier au corps
                body = content;

                // Stocker le nom du fichier dans les en-têtes pour référence
                headers["filename"] = filename;
            }
        }
    }
}


std::string HttpRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}