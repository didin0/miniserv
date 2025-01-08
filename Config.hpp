#pragma once
#include <vector>
#include <iostream>
#include <string>

class Config {
    
    
    public : 
    std::vector < std::string> route;
    std::vector < std::string> method;
    bool is_done(); 

    Config();
    ~Config();
};