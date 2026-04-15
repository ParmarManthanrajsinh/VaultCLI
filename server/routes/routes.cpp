#include "routes/routes.h"

#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace vault::server 
{
    static std::string extract_token(const httplib::Request& req) 
    {
        auto it = req.headers.find("Authorization");
        if (it == req.headers.end()) return "";

        const auto& auth = it->second;
        const std::string prefix = "Bearer ";
        if (auth.substr(0, prefix.size()) == prefix) 
        {
            return auth.substr(prefix.size());
        }
        return "";
    }

    static void json_error(httplib::Response& res, int status, const std::string& message) 
    {
        json body;
        body["success"] = false;
        body["message"] = message;
        res.status = status;
        res.set_content(body.dump(), "application/json");
    }

    static void json_ok(httplib::Response& res, const json& data = json::object()) 
    {
        json body = data;
        body["success"] = true;
        res.status = 200;
        res.set_content(body.dump(), "application/json");
    }

    void setup_routes(httplib::Server& server,
                      AuthManager& auth,
                      StorageManager& storage) 
    {
        server.Post("/register", [&auth](const httplib::Request& req,
                                          httplib::Response& res) 
        {
            try 
            {
                auto body = json::parse(req.body);
                std::string username = body.value("username", "");
                std::string password = body.value("password", "");

                if (username.empty() || password.empty()) 
                {
                    json_error(res, 400, "Username and password are required");
                    return;
                }

                if (username.size() < 3 || password.size() < 4) 
                {
                    json_error(res, 400, "Username (min 3) and password (min 4) too short");
                    return;
                }

                if (auth.register_user(username, password)) 
                {
                    json_ok(res, {{"message", "User registered successfully"}});
                } 
                else 
                {
                    json_error(res, 409, "Username already exists");
                }
            } 
            catch (const std::exception& e) 
            {
                json_error(res, 400, std::string("Invalid request: ") + e.what());
            }
        });

        server.Post("/login", [&auth](const httplib::Request& req,
                                       httplib::Response& res) {
            try 
            {
                auto body = json::parse(req.body);
                std::string username = body.value("username", "");
                std::string password = body.value("password", "");

                auto token = auth.login(username, password);
                if (token) 
                {
                    json_ok(res, {{"token", *token}, {"message", "Login successful"}});
                } 
                else 
                {
                    json_error(res, 401, "Invalid username or password");
                }
            } 
            catch (const std::exception& e) 
            {
                json_error(res, 400, std::string("Invalid request: ") + e.what());
            }
        });

        server.Post("/upload", [&auth, &storage](const httplib::Request& req,
                                                  httplib::Response& res) 
        {
            // Authenticate
            std::string token = extract_token(req);
            auto username = auth.validate_token(token);
            if (!username) 
            {
                json_error(res, 401, "Unauthorized — please login first");
                return;
            }

            // Extract file from multipart form data
            auto it = req.files.find("file");
            if (it == req.files.end()) 
            {
                json_error(res, 400, "No file provided");
                return;
            }

            const auto& file = it->second;
            std::string filename = file.filename;
            if (filename.empty()) 
            {
                json_error(res, 400, "Filename is empty");
                return;
            }

            // Store the encrypted file data (client encrypts before sending)
            std::vector<uint8_t> data(file.content.begin(), file.content.end());
            if (storage.store_file(*username, filename, data)) 
            {
                json_ok(res, {{"message", "File uploaded successfully"},
                              {"filename", filename + ".enc"}});
            } 
            else 
            {
                json_error(res, 500, "Failed to store file");
            }
        });

        server.Get("/download", [&auth, &storage](const httplib::Request& req,
                                                   httplib::Response& res) 
        {
            // Authenticate
            std::string token = extract_token(req);
            auto username = auth.validate_token(token);
            if (!username) 
            {
                json_error(res, 401, "Unauthorized — please login first");
                return;
            }

            // Get filename from query parameter
            std::string filename = req.get_param_value("filename");
            if (filename.empty()) 
            {
                json_error(res, 400, "Filename parameter is required");
                return;
            }

            try 
            {
                auto data = storage.retrieve_file(*username, filename);
                res.status = 200;
                res.set_content(std::string(data.begin(), data.end()),
                               "application/octet-stream");
                res.set_header("Content-Disposition",
                              "attachment; filename=\"" + filename + "\"");
            } 
            catch (const std::exception& e) 
            {
                json_error(res, 404, std::string("File not found: ") + e.what());
            }
        });

        server.Get("/list", [&auth, &storage](const httplib::Request& req,
                                               httplib::Response& res) 
        {
            // Authenticate
            std::string token = extract_token(req);
            auto username = auth.validate_token(token);
            if (!username) 
            {
                json_error(res, 401, "Unauthorized — please login first");
                return;
            }

            auto files = storage.list_files(*username);
            json file_list = json::array();
            for (const auto& f : files) 
            {
                file_list.push_back
                ({
                    {"filename", f.filename},
                    {"size", f.size},
                    {"uploaded_at", f.uploaded_at}
                });
            }

            json_ok(res, {{"files", file_list}, {"count", files.size()}});
        });

        server.Get("/health", [](const httplib::Request&, httplib::Response& res) 
        {
            json_ok(res, {{"status", "running"}});
        });

        std::cout << "[Routes] All API endpoints registered\n";
    }
}