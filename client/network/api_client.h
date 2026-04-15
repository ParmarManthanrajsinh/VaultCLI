#pragma once

#include "models/file_meta.h"

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace vault::client 
{

    /// Result of an API operation
    struct ApiResult 
    {
        bool success = false;
        std::string message;
        std::vector<uint8_t> data;   // Raw response data (for downloads)
    };

    /// HTTP client wrapper for VaultCLI server communication
    class ApiClient 
    {
    public:
        ApiClient(const std::string& host, int port);

        /// Register a new user
        ApiResult register_user(const std::string& username, const std::string& password);

        /// Log in and store the session token
        ApiResult login(const std::string& username, const std::string& password);

        /// Upload a file (encrypts on client side before sending)
        ApiResult upload_file(const std::string& filepath, const std::string& password);

        /// Download a file (decrypts after receiving)
        ApiResult download_file(const std::string& filename,
                                const std::string& dest_path,
                                const std::string& password);

        /// List files stored on the server
        std::vector<models::FileMeta> list_files();

        /// Logout (clear session)
        void logout();

        /// Check if currently authenticated
        bool is_authenticated() const { return !token_.empty(); }

        /// Get the current username
        const std::string& username() const { return username_; }

    private:
        std::string host_;
        int port_;
        std::string token_;
        std::string username_;
    };

} // namespace vault::client
