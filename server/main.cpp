#include "auth/auth_manager.h"
#include "storage/storage_manager.h"
#include "routes/routes.h"

#include <httplib.h>
#include <iostream>
#include <string>
#include <csignal>

static httplib::Server* g_server = nullptr;

static void signal_handler(int) {
    if (g_server) {
        std::cout << "\n[Server] Shutting down...\n";
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    // ── Parse command line arguments ────────────────────────────────────
    int port = 8080;
    std::string host = "0.0.0.0";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if ((arg == "--host" || arg == "-h") && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: vault_server [options]\n"
                      << "  --port, -p <port>  Server port (default: 8080)\n"
                      << "  --host, -h <host>  Bind address (default: 0.0.0.0)\n"
                      << "  --help             Show this help\n";
            return 0;
        }
    }

    // ── Print startup banner ────────────────────────────────────────────
    std::cout << R"(
 ╔══════════════════════════════════════════════════╗
 ║          VaultCLI Server v1.0.0                  ║
 ║     Secure Cloud File Storage                    ║
 ╠══════════════════════════════════════════════════╣
 ║  Encrypted file storage with token-based auth    ║
 ╚══════════════════════════════════════════════════╝
)" << std::endl;

    // ── Initialize components ───────────────────────────────────────────
    vault::server::AuthManager auth("data");
    vault::server::StorageManager storage("storage");

    httplib::Server server;
    g_server = &server;

    // Register signal handler for graceful shutdown (Ctrl+C)
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // ── Setup routes ────────────────────────────────────────────────────
    vault::server::setup_routes(server, auth, storage);

    // ── Start listening ─────────────────────────────────────────────────
    std::cout << "[Server] Listening on " << host << ":" << port << "\n";
    std::cout << "[Server] Press Ctrl+C to stop\n\n";

    if (!server.listen(host, port)) {
        std::cerr << "[Server] Failed to start on " << host << ":" << port << "\n";
        return 1;
    }

    std::cout << "[Server] Stopped\n";
    return 0;
}
