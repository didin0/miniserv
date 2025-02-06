#pragma once 
#include <vector>
#include <string>
#include <iostream>


class ConfigServer
{
  private : 
    std::vector<std::string> _route; 
    std::vector<std::string> _methode; 


  public : 
    ConfigServer(); 
    ConfigServer(ConfigServer const &ref); 
    ConfigServer& operator=(ConfigServer const &ref); 
    ~ConfigServer(); 
};