#pragma once

#include <string>

namespace vault::models 
{
    struct User 
    {
        std::string username;
        std::string password_hash;
        std::string salt;
    };

} 
