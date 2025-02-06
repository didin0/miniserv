#include "ConfigServer.hpp"

ConfigServer::ConfigServer() {
}

ConfigServer::ConfigServer(ConfigServer const &ref) {
    *this = ref;
}

ConfigServer& ConfigServer::operator=(ConfigServer const &ref) {
    if (this != &ref) {
        _route = ref._route;
        _methode = ref._methode;
    }
    return *this;
}

ConfigServer::~ConfigServer() {
}