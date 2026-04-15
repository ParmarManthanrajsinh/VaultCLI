#include "auth/auth_manager.h"
#include "crypto/crypto.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace vault::server 
{

    AuthManager::AuthManager(const std::filesystem::path& data_dir)
        : data_dir_(data_dir)
        , users_file_(data_dir / "users.dat")
    {
        std::filesystem::create_directories(data_dir_);
        load_users();
    }

    void AuthManager::load_users() 
    {
        std::ifstream file(users_file_);
        if (!file.is_open()) return; // No users file yet — first run

        std::string line;
        while (std::getline(file, line)) 
        {
            // Format: username:password_hash:salt
            std::istringstream iss(line);
            std::string username, hash, salt;

            if (std::getline(iss, username, ':') &&
                std::getline(iss, hash, ':')     &&
                std::getline(iss, salt)) 
            {
                users_[username] = models::User{username, hash, salt};
            }
        }
        std::cout << "[Auth] Loaded " << users_.size() << " user(s)\n";
    }

    void AuthManager::save_user(const models::User& user) 
    {
        std::ofstream file(users_file_, std::ios::app);
        if (!file.is_open()) 
        {
            throw std::runtime_error("Cannot write to users file");
        }
        // SECURITY: Credentials stored as hash:salt — plaintext password never written
        file << user.username << ":" << user.password_hash << ":" << user.salt << "\n";
    }

    bool AuthManager::register_user(const std::string& username,
                                     const std::string& password) 
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if user already exists
        if (users_.find(username) != users_.end()) 
        {
            return false;
        }

        // SECURITY: Generate unique salt per user, hash password with salt
        std::string salt = crypto::generate_salt();
        std::string hash = crypto::sha256_hash(password, salt);

        models::User user{username, hash, salt};
        users_[username] = user;
        save_user(user);

        std::cout << "[Auth] Registered user: " << username << "\n";
        return true;
    }

    std::optional<std::string> AuthManager::login(const std::string& username,
                                                   const std::string& password) 
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = users_.find(username);
        if (it == users_.end()) 
        {
            return std::nullopt; // User not found
        }

        // SECURITY: Re-hash the provided password with the stored salt and compare
        const auto& user = it->second;
        std::string hash = crypto::sha256_hash(password, user.salt);

        if (hash != user.password_hash) 
        {
            return std::nullopt; // Wrong password
        }

        // Generate session token
        std::string token = crypto::generate_token();
        sessions_[token] = username;

        std::cout << "[Auth] User logged in: " << username << "\n";
        return token;
    }

    std::optional<std::string> AuthManager::validate_token(const std::string& token) const 
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(token);
        if (it == sessions_.end()) 
        {
            return std::nullopt;
        }
        return it->second;
    }

    void AuthManager::logout(const std::string& token) 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(token);
    }

} 
