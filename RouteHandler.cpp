#include "RouteHandler.hpp"

// Map pour stocker les routes et leurs méthodes autorisées
std::map<std::string, std::vector<std::string> > allowedMethods;

// Ajouter une route avec des méthodes autorisées
void RouteHandler::addRoute(const std::string& route, const std::vector<std::string>& methods) {
    allowedMethods[route] = methods;
}

// Vérifier si une méthode est autorisée pour une route donnée
bool RouteHandler::isMethodAllowed(const std::string& route, const std::string& method) const {
    std::map<std::string, std::vector<std::string> >::const_iterator it = allowedMethods.find(route);
    if (it != allowedMethods.end()) {
        const std::vector<std::string>& methods = it->second;
        for (std::vector<std::string>::const_iterator mit = methods.begin(); mit != methods.end(); ++mit) {
            if (*mit == method) {
                return true;
            }
        }
    }
    return false;
}

// Obtenir les méthodes autorisées pour une route donnée
std::vector<std::string> RouteHandler::getAllowedMethods(const std::string& route) const {
    std::map<std::string, std::vector<std::string> >::const_iterator it = allowedMethods.find(route);
    if (it != allowedMethods.end()) {
        return it->second;
    }
    return std::vector<std::string>(); // Retourne une liste vide si la route n'existe pas
}
