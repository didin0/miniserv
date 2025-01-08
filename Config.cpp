#include "Config.hpp"






Config::Config()
{
    this->route.push_back("index");
    this->route.push_back("about");
    this->route.push_back("404");

    this->method.push_back("GET");
    this->method.push_back("POST");
    this->method.push_back("DELETE");


}


Config::~Config()
{}
bool Config::is_done()
{ return true;}


