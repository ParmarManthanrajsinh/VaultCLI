---

# VaultCLI – Secure Cross-Platform Cloud File Server

A cross-platform terminal-based cloud file storage system written in modern C++ (C++20).  
Users can register, log in, upload files, download files, and list their stored files with **AES-256 encryption** applied to all file contents and **SHA-256 hashing** for passwords.

---

## ✨ Features

| Feature | Details |
|---------|---------|
| **Client–Server Architecture** | HTTP-based communication with REST API |
| **Text User Interface (TUI)** | Built with FTXUI — keyboard-only navigation |
| **File Encryption** | AES-256-CBC client-side encryption via OpenSSL |
| **Password Security** | SHA-256 hashing with unique per-user salt |
| **Session Management** | Token-based authentication (Bearer tokens) |
| **Cross-Platform** | Windows (MSVC/MinGW), Linux (GCC), macOS (Clang) |
| **Per-User Storage** | Isolated file storage per user on the server |

---

## 📁 Project Structure

```
VaultCLI/
├── CMakeLists.txt              # Root build configuration
├── Makefile                    # Build automation (recommended)
├── README.md
├── common/                     # Shared library
│   ├── CMakeLists.txt
│   ├── crypto/                 # SHA-256, AES-256, token generation
│   │   ├── crypto.h
│   │   └── crypto.cpp
│   ├── models/                 # Data structures
│   │   ├── user.h
│   │   └── file_meta.h
│   └── utils/                  # File I/O, timestamps, URL encoding
│       ├── utils.h
│       └── utils.cpp
├── server/                     # Server executable
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── auth/                   # User registration & session management
│   │   ├── auth_manager.h
│   │   └── auth_manager.cpp
│   ├── storage/                # Per-user encrypted file storage
│   │   ├── storage_manager.h
│   │   └── storage_manager.cpp
│   └── routes/                 # HTTP API endpoint handlers
│       ├── routes.h
│       └── routes.cpp
└── client/                     # Client executable
    ├── CMakeLists.txt
    ├── main.cpp
    ├── network/                # HTTP client wrapper
    │   ├── api_client.h
    │   └── api_client.cpp
    └── tui/                    # FTXUI terminal interface
        ├── app.h
        └── app.cpp
```

---

## 🔧 Prerequisites

- **C++ Compiler** with C++20 support:
  - Windows: MSVC (Visual Studio 2019+) or MinGW-w64
  - Linux: GCC 10+ or Clang 12+
  - macOS: Clang 12+ (Xcode Command Line Tools)
- **CMake** 3.20 or later
- **OpenSSL** development libraries
- **Ninja** build system (recommended for speed)
- **Git** (for FetchContent dependencies)

### Installing Dependencies

| Platform | Command |
|----------|---------|
| **Windows** | `choco install openssl ninja` or download manually |
| **Ubuntu/Debian** | `sudo apt install libssl-dev ninja-build` |
| **Fedora/RHEL** | `sudo dnf install openssl-devel ninja-build` |
| **macOS** | `brew install openssl ninja` |

---

## 🚀 Build Instructions

### Quick Build with Make (Recommended)

```bash
cd VaultCLI

# Build release version (optimized)
make release

# Build debug version (with symbols)
make debug

# Clean build directory
make clean

# Show help
make help
```

### Build Output

After building, you'll find two executables:
- `build/server/vault_server` - Server executable
- `build/client/vault_client` - Client executable

> **Windows Note**: Executables will have `.exe` extension

---

## 📦 Makefile Commands Reference

| Command | Description |
|---------|-------------|
| `make release` | Build release version with `-O2 -march=native` optimization |
| `make debug` | Build debug version with debug symbols (`-g -O0`) |
| `make clean` | Remove build directory and all generated files |
| `make help` | Display available commands |

### Makefile Features

- **Cross-platform** – Works on Linux, macOS, and Windows
- **Optimized builds** – `-O2 -march=native -DNDEBUG` for release
- **Automatic cleanup** – Removes old build before rebuilding
- **Color output** – Green for success, yellow for warnings (Linux/macOS)

---

## 🖥️ Usage

### Start the Server

```bash
# Using Make (Linux/macOS only)
make run-server

# Manual
./build/server/vault_server

# Custom port
./build/server/vault_server --port 9000
```

### Start the Client

```bash
# Using Make (Linux/macOS only)
make run-client

# Manual
./build/client/vault_client

# Connect to custom server
./build/client/vault_client --host 192.168.1.100 --port 9000
```

### Command Line Options

| Executable | Option | Default | Description |
|------------|--------|---------|-------------|
| `vault_server` | `--port, -p` | `8080` | Server listen port |
| `vault_server` | `--host, -h` | `0.0.0.0` | Bind address |
| `vault_client` | `--host, -H` | `localhost` | Server hostname |
| `vault_client` | `--port, -p` | `8080` | Server port |

---

## 📋 Example Workflow

1. **Start the server** in Terminal 1:
   ```
   make release
   ./build/server/vault_server --port 8080
   [Server] Listening on 0.0.0.0:8080
   ```

2. **Start the client** in Terminal 2:
   ```
   ./build/client/vault_client
   ```

3. **Register** a new account:
   - Switch to "Register" mode using the toggle
   - Enter username and password
   - Press Submit

4. **Upload a file**:
   - Select "Upload File" from the dashboard
   - Enter the full path to your file
   - Enter an encryption password (remember this!)
   - The file is encrypted client-side, then sent to the server

5. **List files**:
   - Select "List Files" to see all your uploaded files
   - Files appear with `.enc` extension, size, and upload date

6. **Download a file**:
   - Select "Download File"
   - Enter the filename (as shown in the file list)
   - Enter the save directory
   - Enter the same encryption password used during upload
   - The file is downloaded, decrypted, and saved locally

---

## 🔐 Security Design

### Password Security
- Passwords are **never stored in plaintext**
- Each user gets a unique **16-byte random salt** (CSPRNG)
- Passwords are hashed as `SHA-256(salt + password)`
- Stored format: `username:hash:salt` in `data/users.dat`

### File Encryption
- Files encrypted on the **client side** before upload
- Algorithm: **AES-256-CBC** with PKCS7 padding
- A random **16-byte IV** is generated per encryption
- IV is prepended to ciphertext for extraction during decryption
- Encryption key derived via `SHA-256(user_password)`
- Server **only stores encrypted `.enc` files** — cannot read contents

### Session Management
- Login produces a **64-character hex token** (32 random bytes)
- Token sent via `Authorization: Bearer <token>` header
- Sessions stored in memory (cleared on server restart)

---

## 📡 HTTP API Reference

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/register` | `POST` | No | Register new user (`{username, password}`) |
| `/login` | `POST` | No | Authenticate (`{username, password}` → `{token}`) |
| `/upload` | `POST` | Bearer | Upload encrypted file (multipart form) |
| `/download` | `GET` | Bearer | Download encrypted file (`?filename=X`) |
| `/list` | `GET` | Bearer | List user's files (JSON array) |
| `/health` | `GET` | No | Server health check |

---

## 📚 Libraries Used

| Library | Version | Purpose |
|---------|---------|---------|
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | v0.15.3 | HTTP client & server |
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | v5.0.0 | Terminal UI framework |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.11.3 | JSON serialization |
| [OpenSSL](https://www.openssl.org/) | System | Cryptography (AES, SHA) |

---
