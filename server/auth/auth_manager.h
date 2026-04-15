#pragma once

#include "models/user.h"

#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <filesystem>

namespace vault::server 
{

    /// Manages user registration, authentication, and session tokens
    class AuthManager 
    {
    public:
        explicit AuthManager(const std::filesystem::path& data_dir = "data");

        /// Register a new user. Returns false if username already exists.
        bool register_user(const std::string& username, const std::string& password);

        /// Authenticate user. Returns session token on success.
        std::optional<std::string> login(const std::string& username,
                                         const std::string& password);

        /// Validate a session token. Returns the username if valid.
        std::optional<std::string> validate_token(const std::string& token) const;

        /// Remove a session token (logout)
        void logout(const std::string& token);

    private:
        void load_users();
        void save_user(const models::User& user);

        std::filesystem::path data_dir_;
        std::filesystem::path users_file_;

        std::unordered_map<std::string, models::User> users_;      // username → User
        std::unordered_map<std::string, std::string> sessions_;    // token → username

        mutable std::mutex mutex_;
    };

} 
