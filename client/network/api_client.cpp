#include "network/api_client.h"
#include "crypto/crypto.h"
#include "utils/utils.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <filesystem>
using json = nlohmann::json;

namespace vault::client
{
    ApiClient::ApiClient(const std::string& host, int port)
        : host_(host), port_(port) 
    {
    }

    ApiResult ApiClient::register_user(const std::string& username,
                                        const std::string& password)
    {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5);
        cli.set_read_timeout(10);

        json body;
        body["username"] = username;
        body["password"] = password;

        auto result = cli.Post("/register", body.dump(), "application/json");
        if (!result)
        {
            return {false, "Cannot connect to server"};
        }

        auto response = json::parse(result->body, nullptr, false);
        if (response.is_discarded())
        {
            return {false, "Invalid server response"};
        }

        return 
        {
            response.value("success", false),
            response.value("message", "Unknown error")
        };
    }

    ApiResult ApiClient::login(const std::string& username,
                                const std::string& password)
    {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5);
        cli.set_read_timeout(10);

        json body;
        body["username"] = username;
        body["password"] = password;

        auto res = cli.Post("/login", body.dump(), "application/json");
        if (!res)
        {
            return {false, "Cannot connect to server"};
        }

        auto resp = json::parse(res->body, nullptr, false);
        if (resp.is_discarded())
        {
            return {false, "Invalid server response"};
        }

        bool success = resp.value("success", false);
        if (success)
        {
            token_ = resp.value("token", "");
            username_ = username;
        }

        return 
        {
            success,
            resp.value("message", "Unknown error")
        };
    }

    ApiResult ApiClient::upload_file(const std::string& filepath,
                                      const std::string& password)
    {
        if (token_.empty())
        {
            return {false, "Not authenticated"};
        }

        // Read the file
        std::vector<uint8_t> file_data;
        try
        {
            file_data = utils::read_file_binary(filepath);
        }
        catch (const std::exception& e)
        {
            return {false, std::string("Cannot read file: ") + e.what()};
        }

        // SECURITY: Encrypt the file on the client side before sending
        std::vector<uint8_t> encrypted;
        try
        {
            encrypted = crypto::aes256_encrypt(file_data, password);
        }
        catch (const std::exception& e)
        {
            return {false, std::string("Encryption failed: ") + e.what()};
        }

        // Send as multipart form data
        std::string filename = utils::extract_filename(filepath);

        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5);
        cli.set_read_timeout(30);

        httplib::MultipartFormDataItems items = 
        {
            {"file", std::string(encrypted.begin(), encrypted.end()),
             filename, "application/octet-stream"}
        };

        httplib::Headers headers = 
        {
            {"Authorization", "Bearer " + token_}
        };

        auto res = cli.Post("/upload", headers, items, httplib::MultipartFormDataProviderItems{});
        if (!res)
        {
            return {false, "Cannot connect to server"};
        }

        auto resp = json::parse(res->body, nullptr, false);
        if (resp.is_discarded())
        {
            return {false, "Invalid server response"};
        }

        return 
        {
            resp.value("success", false),
            resp.value("message", "Unknown error")
        };
    }

    ApiResult ApiClient::download_file(const std::string& filename,
                                        const std::string& dest_path,
                                        const std::string& password)
    {
        if (token_.empty())
        {
            return {false, "Not authenticated"};
        }

        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5);
        cli.set_read_timeout(30);

        httplib::Headers headers = 
        {
            {"Authorization", "Bearer " + token_}
        };

        // The server stores files with .enc extension
        std::string enc_filename = filename;
        if (enc_filename.find(".enc") == std::string::npos)
        {
            enc_filename += ".enc";
        }

        auto res = cli.Get("/download?filename=" + enc_filename, headers);
        if (!res)
        {
            return {false, "Cannot connect to server"};
        }

        if (res->status != 200)
        {
            auto resp = json::parse(res->body, nullptr, false);
            return {false, resp.is_object() ? resp.value("message", "Download failed")
                                            : "Download failed"};
        }

        // SECURITY: Decrypt the file after downloading from server
        std::vector<uint8_t> encrypted(res->body.begin(), res->body.end());
        std::vector<uint8_t> decrypted;
        try
        {
            decrypted = crypto::aes256_decrypt(encrypted, password);
        }
        catch (const std::exception& e)
        {
            return {false, std::string("Decryption failed: ") + e.what()};
        }

        try
        {
            // Remove .enc extension for the output filename if present
            std::string output_name = filename;
            if (output_name.size() > 4 &&
                output_name.substr(output_name.size() - 4) == ".enc")
            {
                output_name = output_name.substr(0, output_name.size() - 4);
            }

            std::filesystem::path dest(dest_path);
            if (std::filesystem::is_directory(dest))
            {
                dest = dest / output_name;
            }

            utils::write_file_binary(dest, decrypted);
            return {true, "File downloaded and decrypted: " + dest.string()};
        }
        catch (const std::exception& e)
        {
            return {false, std::string("Cannot save file: ") + e.what()};
        }
    }

    std::vector<models::FileMeta> ApiClient::list_files()
    {
        std::vector<models::FileMeta> files;

        if (token_.empty()) return files;

        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5);
        cli.set_read_timeout(10);

        httplib::Headers headers = 
        {
            {"Authorization", "Bearer " + token_}
        };

        auto res = cli.Get("/list", headers);
        if (!res || res->status != 200) return files;

        auto resp = json::parse(res->body, nullptr, false);
        if (resp.is_discarded() || !resp.contains("files")) return files;

        for (const auto& f : resp["files"])
        {
            models::FileMeta meta;
            meta.filename = f.value("filename", "");
            meta.size = f.value("size", static_cast<size_t>(0));
            meta.uploaded_at = f.value("uploaded_at", "");
            files.push_back(std::move(meta));
        }

        return files;
    }

    void ApiClient::logout()
    {
        token_.clear();
        username_.clear();
    }
}