#ifndef HTTPUTILS_HPP
#define HTTPUTILS_HPP

#include <string>
#include <sstream>
#include <string>

std::string getMimeType(const std::string& filename);
std::string extractBody(const std::string& request);

#endif // HTTPUTILS_HPP