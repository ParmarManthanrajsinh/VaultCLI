#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace vault::crypto 
{

    // ─── Password Hashing ───────────────────────────────────────────────────────

    /// Generate a cryptographically random salt (16 bytes, hex-encoded)
    std::string generate_salt();

    /// Hash a password with a salt using SHA-256
    /// Returns hex-encoded hash string
    std::string sha256_hash(const std::string& password, const std::string& salt);

    // ─── AES-256-CBC File Encryption ─────────────────────────────────────────────

    /// Generate a 32-byte AES key derived from a password using SHA-256
    /// The password is hashed to produce a consistent 256-bit key
    std::vector<uint8_t> derive_aes_key(const std::string& password);

    /// Generate a random 16-byte initialization vector
    std::vector<uint8_t> generate_iv();

    /// Encrypt plaintext data using AES-256-CBC
    /// IV is prepended to the ciphertext for later extraction during decryption
    std::vector<uint8_t> aes256_encrypt(const std::vector<uint8_t>& plaintext,
                                         const std::string& password);

    std::vector<uint8_t> aes256_decrypt(const std::vector<uint8_t>& ciphertext,
                                         const std::string& password);
    std::string generate_token();
}
