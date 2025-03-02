#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

class ExceptionHandler
{
public:
    ExceptionHandler() = default;

    void Handle(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    void Throw(const std::string& msg)
    {
        throw std::runtime_error(msg);
    }
};