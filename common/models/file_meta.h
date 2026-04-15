#pragma once

#include <string>
#include <cstddef>

namespace vault::models 
{
    struct FileMeta 
    {
        std::string filename;
        std::size_t size = 0;         // File size in bytes
        std::string uploaded_at;      // ISO timestamp
    };
}
