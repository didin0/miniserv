#ifndef ROUTEHANDLER_HPP
#define ROUTEHANDLER_HPP

#include <string>
#include <vector>
#include <map>

class RouteHandler {
private:
    // Map pour stocker les routes et leurs méthodes autorisées
    std::map<std::string, std::vector<std::string> > allowedMethods;

public:
    // Ajouter une route avec des méthodes autorisées
    void addRoute(const std::string& route, const std::vector<std::string>& methods);
    // Vérifier si une méthode est autorisée pour une route donnée
    bool isMethodAllowed(const std::string& route, const std::string& method) const;
    // Obtenir les méthodes autorisées pour une route donnée
    std::vector<std::string> getAllowedMethods(const std::string& route) const;
};
#endif
