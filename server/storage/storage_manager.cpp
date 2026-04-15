#include "storage/storage_manager.h"
#include "utils/utils.h"

#include <iostream>
#include <chrono>

namespace vault::server 
{
    StorageManager::StorageManager(const std::filesystem::path& storage_dir)
        : storage_dir_(storage_dir)
    {
        std::filesystem::create_directories(storage_dir_);
        std::cout << "[Storage] Storage directory: " << storage_dir_.string() << "\n";
    }

    std::filesystem::path StorageManager::get_user_dir(const std::string& username) const 
    {
        return storage_dir_ / username;
    }

    std::filesystem::path StorageManager::get_file_path(const std::string& username,
                                                          const std::string& filename) const 
    {
        // Ensure all stored files have .enc extension
        std::string enc_name = filename;
        if (enc_name.find(".enc") == std::string::npos) 
        {
            enc_name += ".enc";
        }
        return get_user_dir(username) / enc_name;
    }

    bool StorageManager::store_file(const std::string& username,
                                     const std::string& filename,
                                     const std::vector<uint8_t>& data) 
    {
        try 
        {
            auto user_dir = get_user_dir(username);
            std::filesystem::create_directories(user_dir);

            auto file_path = get_file_path(username, filename);
            utils::write_file_binary(file_path, data);

            std::cout << "[Storage] Stored file: " << file_path.string()
                      << " (" << data.size() << " bytes)\n";
            return true;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "[Storage] Error storing file: " << e.what() << "\n";
            return false;
        }
    }

    std::vector<uint8_t> StorageManager::retrieve_file(const std::string& username,
                                                          const std::string& filename) 
    {
        auto file_path = get_file_path(username, filename);

        if (!std::filesystem::exists(file_path)) 
        {
            throw std::runtime_error("File not found: " + filename);
        }

        return utils::read_file_binary(file_path);
    }

    std::vector<models::FileMeta> StorageManager::list_files(const std::string& username) 
    {
        std::vector<models::FileMeta> files;
        auto user_dir = get_user_dir(username);

        if (!std::filesystem::exists(user_dir)) 
        {
            return files; // No files yet
        }

        for (const auto& entry : std::filesystem::directory_iterator(user_dir)) 
        {
            if (entry.is_regular_file()) 
            {
                models::FileMeta meta;
                meta.filename = entry.path().filename().string();
                meta.size = entry.file_size();

                // Get last write time as a readable timestamp
                auto ftime = entry.last_write_time();
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>
                (
                    ftime - std::filesystem::file_time_type::clock::now()
                    + std::chrono::system_clock::now()
                );
                auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
                struct tm tm_buf;
    #ifdef _WIN32
                localtime_s(&tm_buf, &time_t_val);
    #else
                localtime_r(&time_t_val, &tm_buf);
    #endif
                char buf[32];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
                meta.uploaded_at = buf;

                files.push_back(std::move(meta));
            }
        }

        return files;
    }

    bool StorageManager::file_exists(const std::string& username,
                                      const std::string& filename) const 
    {
        return std::filesystem::exists(get_file_path(username, filename));
    }

} 
