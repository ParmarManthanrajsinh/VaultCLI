#include "crypto/crypto.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cstring>

namespace vault::crypto
{

    // bytes to hex string 
    static std::string to_hex(const unsigned char* data, size_t len)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < len; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    static std::vector<uint8_t> from_hex(const std::string& hex)
    {
        std::vector<uint8_t> bytes;
        bytes.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2)
        {
            uint8_t byte = static_cast<uint8_t>(
                std::stoi(hex.substr(i, 2), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }

    // ─── Password Hashing ───────────────────────────────────────────────────────

    std::string generate_salt()
    {
        unsigned char salt[16];
        // SECURITY: Use OpenSSL's CSPRNG for salt generation
        if (RAND_bytes(salt, sizeof(salt)) != 1)
        {
            throw std::runtime_error("Failed to generate random salt");
        }
        return to_hex(salt, sizeof(salt));
    }

    std::string sha256_hash(const std::string& password, const std::string& salt)
    {
        // SECURITY: Concatenate salt + password to prevent rainbow table attacks
        std::string input = salt + password;

        unsigned char hash[SHA256_DIGEST_LENGTH];
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");

        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
            EVP_DigestUpdate(ctx, input.c_str(), input.size()) != 1 ||
            EVP_DigestFinal_ex(ctx, hash, nullptr) != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("SHA-256 hashing failed");
        }

        EVP_MD_CTX_free(ctx);
        return to_hex(hash, SHA256_DIGEST_LENGTH);
    }

    // ─── AES-256-CBC Encryption ─────────────────────────────────────────────────

    std::vector<uint8_t> derive_aes_key(const std::string& password)
    {
        // SECURITY: Derive a 256-bit key from the password using SHA-256
        // This ensures the key is always exactly 32 bytes regardless of password length
        unsigned char hash[SHA256_DIGEST_LENGTH];
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");

        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
            EVP_DigestUpdate(ctx, password.c_str(), password.size()) != 1 ||
            EVP_DigestFinal_ex(ctx, hash, nullptr) != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Key derivation failed");
        }

        EVP_MD_CTX_free(ctx);
        return std::vector<uint8_t>(hash, hash + SHA256_DIGEST_LENGTH);
    }

    std::vector<uint8_t> generate_iv()
    {
        std::vector<uint8_t> iv(16);
        // SECURITY: Use CSPRNG for IV — each encryption must use a unique IV
        if (RAND_bytes(iv.data(), 16) != 1)
        {
            throw std::runtime_error("Failed to generate IV");
        }
        return iv;
    }

    std::vector<uint8_t> aes256_encrypt(const std::vector<uint8_t>& plaintext,
                                         const std::string& password)
    {
        auto key = derive_aes_key(password);
        auto iv = generate_iv();

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        // SECURITY: AES-256-CBC with PKCS7 padding (default in OpenSSL)
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                               key.data(), iv.data()) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Encryption init failed");
        }

        // Output buffer: plaintext size + one extra block for padding
        std::vector<uint8_t> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        int out_len = 0;
        int total_len = 0;

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                              plaintext.data(), static_cast<int>(plaintext.size())) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Encryption update failed");
        }
        total_len = out_len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Encryption finalize failed");
        }
        total_len += out_len;
        ciphertext.resize(total_len);

        EVP_CIPHER_CTX_free(ctx);

        // SECURITY: Prepend the IV to the ciphertext so it can be extracted during decryption
        // IV does not need to be secret, but must be unique per encryption
        std::vector<uint8_t> result;
        result.reserve(iv.size() + ciphertext.size());
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());
        return result;
    }

    std::vector<uint8_t> aes256_decrypt(const std::vector<uint8_t>& ciphertext,
                                         const std::string& password)
    {
        if (ciphertext.size() < 16)
        {
            throw std::runtime_error("Ciphertext too short — missing IV");
        }

        auto key = derive_aes_key(password);

        // SECURITY: Extract the IV from the first 16 bytes of ciphertext
        std::vector<uint8_t> iv(ciphertext.begin(), ciphertext.begin() + 16);
        const uint8_t* enc_data = ciphertext.data() + 16;
        int enc_len = static_cast<int>(ciphertext.size() - 16);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                               key.data(), iv.data()) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption init failed");
        }

        std::vector<uint8_t> plaintext(enc_len + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        int out_len = 0;
        int total_len = 0;

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                              enc_data, enc_len) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption update failed");
        }
        total_len = out_len;

        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption failed — wrong password or corrupted data");
        }
        total_len += out_len;
        plaintext.resize(total_len);

        EVP_CIPHER_CTX_free(ctx);
        return plaintext;
    }

    // ─── Token Generation ────────────────────────────────────────────────────────

    std::string generate_token()
    {
        unsigned char token[32];
        // SECURITY: Use CSPRNG for session token generation
        if (RAND_bytes(token, sizeof(token)) != 1)
        {
            throw std::runtime_error("Failed to generate token");
        }
        return to_hex(token, sizeof(token));
    }

}