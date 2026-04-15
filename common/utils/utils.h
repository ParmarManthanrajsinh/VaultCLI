#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace vault::utils 
{

/// Read an entire file into a byte vector
    std::vector<uint8_t> read_file_binary(const std::filesystem::path& path);
    
    /// Write a byte vector to a file
    void write_file_binary(const std::filesystem::path& path,
                           const std::vector<uint8_t>& data);
    
    /// Get current timestamp as ISO 8601 string
    std::string get_timestamp();
    
    /// URL-encode a string
    std::string url_encode(const std::string& value);
    
    /// URL-decode a string
    std::string url_decode(const std::string& value);
    
    /// Get the filename from a path string
    std::string extract_filename(const std::string& path);

} // namespace vault::utils
