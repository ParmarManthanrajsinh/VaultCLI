#include "utils/utils.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <ctime>

namespace vault::utils
{

    std::vector<uint8_t> read_file_binary(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file: " + path.string());
        }

        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data.data()), size))
        {
            throw std::runtime_error("Failed to read file: " + path.string());
        }

        return data;
    }

    void write_file_binary(const std::filesystem::path& path,
                           const std::vector<uint8_t>& data)
    {
        // Ensure parent directory exists
        if (path.has_parent_path())
        {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file for writing: " + path.string());
        }

        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }

    std::string get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm tm_buf;

    #ifdef _WIN32
        localtime_s(&tm_buf, &time_t_now);
    #else
        localtime_r(&time_t_now, &tm_buf);
    #endif

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
        return oss.str();
    }

    std::string url_encode(const std::string& value)
    {
        std::ostringstream encoded;
        encoded << std::hex << std::uppercase;

        for (unsigned char c : value)
        {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                encoded << c;
            }
            else
            {
                encoded << '%' << std::setw(2) << std::setfill('0')
                        << static_cast<int>(c);
            }
        }
        return encoded.str();
    }

    std::string url_decode(const std::string& value)
    {
        std::string decoded;
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i] == '%' && i + 2 < value.size())
            {
                int hex = std::stoi(value.substr(i + 1, 2), nullptr, 16);
                decoded += static_cast<char>(hex);
                i += 2;
            }
            else if (value[i] == '+')
            {
                decoded += ' ';
            }
            else
            {
                decoded += value[i];
            }
        }
        return decoded;
    }

    std::string extract_filename(const std::string& path)
    {
        return std::filesystem::path(path).filename().string();
    }

}