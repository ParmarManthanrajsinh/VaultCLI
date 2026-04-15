#pragma once

#include "models/file_meta.h"

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace vault::server 
{
    class StorageManager 
    {
    public:
        explicit StorageManager(const std::filesystem::path& storage_dir = "storage");
    
        /// Store encrypted file data for a user
        bool store_file(const std::string& username,
                        const std::string& filename,
                        const std::vector<uint8_t>& data);
        
        /// Retrieve encrypted file data for a user
        std::vector<uint8_t> retrieve_file(const std::string& username,
                                            const std::string& filename);
        
        /// List all files stored for a user
        std::vector<models::FileMeta> list_files(const std::string& username);
        
        /// Check if a file exists for a user
        bool file_exists(const std::string& username,
                         const std::string& filename) const;
        
    private:
        std::filesystem::path get_user_dir(const std::string& username) const;
        std::filesystem::path get_file_path(const std::string& username,
                                             const std::string& filename) const;
        
        std::filesystem::path storage_dir_;
    };

}