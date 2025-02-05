#pragma once 
#include "HttpServer.hpp"
#include <string>
#include <iostream>

class ClientInfo
{
    public : 
    HttpServer* server;
    std::string read_buffer;
    std::string write_buffer;
    size_t write_offset;



    ClientInfo();
    ~ClientInfo();
};