#include "tui/app.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>

#include <vector>
#include <string>
#include <functional>
#include <memory>

using namespace ftxui;

namespace vault::client
{
    static Color primary()
    {
        return Color::RGB(100, 149, 237);  // Cornflower blue
    }

    static Color accent()
    {
        return Color::RGB(72, 209, 204);   // Medium turquoise
    }

    static Color success_c()
    {
        return Color::RGB(50, 205, 50);    // Lime green
    }

    static Color error_c()
    {
        return Color::RGB(255, 99, 71);    // Tomato
    }

    static Color dim()
    {
        return Color::RGB(128, 128, 128);  // Gray
    }

    // ─── Styled box helper ──────────────────────────────────────────────────────

    static Element styled_box(const std::string& title, Element content)
    {
        return vbox({
            text(title) | bold | color(primary()) | hcenter,
            separator() | color(dim()),
            std::move(content),
        }) | borderRounded | color(accent());
    }

    // ─── App Implementation ─────────────────────────────────────────────────────

    App::App(std::shared_ptr<ApiClient> api) : api_(std::move(api)) {}

    void App::run()
    {
        auto screen = ScreenInteractive::Fullscreen();

        // ── Shared state ────────────────────────────────────────────────────
        int current_screen_index = 0; // 0=LOGIN, 1=DASHBOARD, 2=UPLOAD, 3=DOWNLOAD, 4=FILES

        // Login state
        std::string login_username;
        std::string login_password;
        std::string status_message;
        bool status_is_error = false;
        int login_tab = 0; // 0 = Login, 1 = Register

        // Dashboard state
        int dashboard_selected = 0;

        // Upload state
        std::string upload_path;
        std::string upload_key;

        // Download state
        std::string download_filename;
        std::string download_dest;
        std::string download_key;

        // Files state
        std::vector<models::FileMeta> file_list;

        // Helper to set status
        auto set_status = [&](const std::string& msg, bool is_error)
        {
            status_message = msg;
            status_is_error = is_error;
        };

        // ── Login/Register Input Components ─────────────────────────────────
        auto input_username = Input(&login_username, "Username");
        InputOption opt_password;
        opt_password.password = true;
        auto input_password = Input(&login_password, "Password", opt_password);

        std::vector<std::string> login_tabs = {"Login", "Register"};
        auto tab_toggle = Toggle(&login_tabs, &login_tab);

        auto login_button = Button("  Submit  ", [&]
        {
            if (login_username.empty() || login_password.empty())
            {
                set_status("Please enter username and password", true);
                return;
            }

            if (login_tab == 0)
            {
                // Login
                auto result = api_->login(login_username, login_password);
                if (result.success)
                {
                    set_status("", false);
                    current_screen_index = 1; // DASHBOARD
                    login_password.clear();
                }
                else
                {
                    set_status(result.message, true);
                }
            }
            else
            {
                // Register
                auto result = api_->register_user(login_username, login_password);
                if (result.success)
                {
                    set_status("Registered! Logging in...", false);
                    // Auto-login after registration
                    auto login_result = api_->login(login_username, login_password);
                    if (login_result.success)
                    {
                        current_screen_index = 1; // DASHBOARD
                        login_password.clear();
                    }
                    else
                    {
                        set_status("Registered but login failed: " + login_result.message, true);
                    }
                }
                else
                {
                    set_status(result.message, true);
                }
            }
        }, ButtonOption::Animated(Color::RGB(100, 149, 237)));

        auto login_container = Container::Vertical({
            tab_toggle,
            input_username,
            input_password,
            login_button,
        });

        auto login_renderer = Renderer(login_container, [&]
        {
            auto status_element = text("");
            if (!status_message.empty())
            {
                status_element = text(status_message)
                    | color(status_is_error ? error_c() : success_c())
                    | hcenter;
            }

            return vbox({
                filler(),
                hbox({
                    filler(),
                    vbox({
                        text("") | size(HEIGHT, EQUAL, 1),
                        text("  ╔═══════════════════════════════════╗  ") | color(primary()) | bold,
                        text("  ║       🔒 VaultCLI Client          ║  ") | color(primary()) | bold,
                        text("  ║   Secure Cloud File Storage       ║  ") | color(primary()) | bold,
                        text("  ╚═══════════════════════════════════╝  ") | color(primary()) | bold,
                        text("") | size(HEIGHT, EQUAL, 1),
                        styled_box(login_tab == 0 ? "Login" : "Register", vbox({
                            hbox({text("  Mode:     "), tab_toggle->Render()}) | color(accent()),
                            separator() | color(dim()),
                            hbox({text("  User:     "), input_username->Render() | flex}) | size(WIDTH, GREATER_THAN, 35),
                            hbox({text("  Pass:     "), input_password->Render() | flex}) | size(WIDTH, GREATER_THAN, 35),
                            text("") | size(HEIGHT, EQUAL, 1),
                            login_button->Render() | hcenter,
                            text("") | size(HEIGHT, EQUAL, 1),
                            status_element,
                        })),
                        text("") | size(HEIGHT, EQUAL, 1),
                        text("Tab to switch fields • Enter to submit") | color(dim()) | hcenter,
                    }) | size(WIDTH, LESS_THAN, 55),
                    filler(),
                }),
                filler(),
            });
        });

        // ── Dashboard Components ────────────────────────────────────────────
        std::vector<std::string> menu_entries = {
            "  📤  Upload File       ",
            "  📥  Download File     ",
            "  📋  List Files        ",
            "  🚪  Logout            ",
            "  ❌  Exit              ",
        };

        auto dashboard_menu = Menu(&menu_entries, &dashboard_selected);

        auto dashboard_container = Container::Vertical({
            dashboard_menu,
        });

        // Handle dashboard selection
        auto dashboard_renderer = Renderer(dashboard_container, [&]
        {
            return vbox({
                filler(),
                hbox({
                    filler(),
                    vbox({
                        text("") | size(HEIGHT, EQUAL, 1),
                        text("  ╔═══════════════════════════════════╗  ") | color(primary()) | bold,
                        text("  ║       🔒 VaultCLI Dashboard       ║  ") | color(primary()) | bold,
                        text("  ╚═══════════════════════════════════╝  ") | color(primary()) | bold,
                        text("") | size(HEIGHT, EQUAL, 1),
                        hbox({text("  Logged in as: "),
                              text(api_->username()) | bold | color(accent())}) | hcenter,
                        text("") | size(HEIGHT, EQUAL, 1),
                        styled_box("Main Menu", vbox({
                            dashboard_menu->Render(),
                        })),
                        text("") | size(HEIGHT, EQUAL, 1),
                        text("Arrow keys to navigate • Enter to select") | color(dim()) | hcenter,
                    }) | size(WIDTH, LESS_THAN, 55),
                    filler(),
                }),
                filler(),
            });
        }) | CatchEvent([&](Event event)
        {
            if (event == Event::Return)
            {
                switch (dashboard_selected)
                {
                    case 0:
                        current_screen_index = 2; // UPLOAD
                        upload_path.clear();
                        upload_key.clear();
                        set_status("", false);
                        return true;
                    case 1:
                        current_screen_index = 3; // DOWNLOAD
                        download_filename.clear();
                        download_dest = ".";
                        download_key.clear();
                        set_status("", false);
                        return true;
                    case 2:
                        current_screen_index = 4; // FILES
                        file_list = api_->list_files();
                        set_status("", false);
                        return true;
                    case 3:
                        api_->logout();
                        current_screen_index = 0; // LOGIN
                        login_username.clear();
                        login_password.clear();
                        set_status("Logged out", false);
                        return true;
                    case 4:
                        screen.Exit();
                        return true;
                }
            }
            if (event == Event::Escape)
            {
                screen.Exit();
                return true;
            }
            return false;
        });

        // ── Upload Screen ───────────────────────────────────────────────────
        auto input_upload_path = Input(&upload_path, "Path to file (e.g. C:\\docs\\file.txt)");
        InputOption opt_upload_key;
        opt_upload_key.password = true;
        auto input_upload_key  = Input(&upload_key, "Encryption password", opt_upload_key);

        auto upload_submit = Button("  Upload  ", [&]
        {
            if (upload_path.empty())
            {
                set_status("Please enter a file path", true);
                return;
            }
            if (upload_key.empty())
            {
                set_status("Please enter an encryption password", true);
                return;
            }

            set_status("Encrypting and uploading...", false);
            auto result = api_->upload_file(upload_path, upload_key);
            set_status(result.message, !result.success);
            if (result.success)
            {
                upload_path.clear();
                upload_key.clear();
            }
        }, ButtonOption::Animated(Color::RGB(50, 205, 50)));

        auto upload_back = Button("  Back  ", [&]
        {
            current_screen_index = 1; // DASHBOARD
            set_status("", false);
        }, ButtonOption::Animated(Color::RGB(255, 99, 71)));

        auto upload_container = Container::Vertical({
            input_upload_path,
            input_upload_key,
            Container::Horizontal({upload_submit, upload_back}),
        });

        auto upload_renderer = Renderer(upload_container, [&]
        {
            auto status_element = text("");
            if (!status_message.empty())
            {
                status_element = text(status_message)
                    | color(status_is_error ? error_c() : success_c())
                    | hcenter;
            }

            return vbox({
                filler(),
                hbox({
                    filler(),
                    styled_box("📤 Upload File", vbox({
                        hbox({text("  File:  "), input_upload_path->Render() | flex})
                            | size(WIDTH, GREATER_THAN, 45),
                        hbox({text("  Key:   "), input_upload_key->Render() | flex})
                            | size(WIDTH, GREATER_THAN, 45),
                        text("") | size(HEIGHT, EQUAL, 1),
                        hbox({
                            upload_submit->Render(),
                            text("  "),
                            upload_back->Render(),
                        }) | hcenter,
                        text("") | size(HEIGHT, EQUAL, 1),
                        status_element,
                    })) | size(WIDTH, LESS_THAN, 60),
                    filler(),
                }),
                filler(),
            });
        });

        // ── Download Screen ─────────────────────────────────────────────────
        auto input_dl_filename = Input(&download_filename, "Filename (e.g. file.txt.enc)");
        auto input_dl_dest     = Input(&download_dest, "Save to directory (e.g. .)");
        InputOption opt_dl_key;
        opt_dl_key.password = true;
        auto input_dl_key      = Input(&download_key, "Decryption password", opt_dl_key);

        auto download_submit = Button("  Download  ", [&]
        {
            if (download_filename.empty())
            {
                set_status("Please enter a filename", true);
                return;
            }
            if (download_key.empty())
            {
                set_status("Please enter the decryption password", true);
                return;
            }

            set_status("Downloading and decrypting...", false);
            auto result = api_->download_file(download_filename, download_dest, download_key);
            set_status(result.message, !result.success);
        }, ButtonOption::Animated(Color::RGB(50, 205, 50)));

        auto download_back = Button("  Back  ", [&]
        {
            current_screen_index = 1; // DASHBOARD
            set_status("", false);
        }, ButtonOption::Animated(Color::RGB(255, 99, 71)));

        auto download_container = Container::Vertical({
            input_dl_filename,
            input_dl_dest,
            input_dl_key,
            Container::Horizontal({download_submit, download_back}),
        });

        auto download_renderer = Renderer(download_container, [&]
        {
            auto status_element = text("");
            if (!status_message.empty())
            {
                status_element = text(status_message)
                    | color(status_is_error ? error_c() : success_c())
                    | hcenter;
            }

            return vbox({
                filler(),
                hbox({
                    filler(),
                    styled_box("📥 Download File", vbox({
                        hbox({text("  File:  "), input_dl_filename->Render() | flex})
                            | size(WIDTH, GREATER_THAN, 45),
                        hbox({text("  Save:  "), input_dl_dest->Render() | flex})
                            | size(WIDTH, GREATER_THAN, 45),
                        hbox({text("  Key:   "), input_dl_key->Render() | flex})
                            | size(WIDTH, GREATER_THAN, 45),
                        text("") | size(HEIGHT, EQUAL, 1),
                        hbox({
                            download_submit->Render(),
                            text("  "),
                            download_back->Render(),
                        }) | hcenter,
                        text("") | size(HEIGHT, EQUAL, 1),
                        status_element,
                    })) | size(WIDTH, LESS_THAN, 60),
                    filler(),
                }),
                filler(),
            });
        });

        // ── File List Screen ────────────────────────────────────────────────
        auto files_refresh = Button("  Refresh  ", [&]
        {
            file_list = api_->list_files();
            set_status("File list refreshed", false);
        }, ButtonOption::Animated(Color::RGB(100, 149, 237)));

        auto files_back = Button("  Back  ", [&]
        {
            current_screen_index = 1; // DASHBOARD
            set_status("", false);
        }, ButtonOption::Animated(Color::RGB(255, 99, 71)));

        auto files_container = Container::Horizontal({
            files_refresh,
            files_back,
        });

        auto files_renderer = Renderer(files_container, [&]
        {
            Elements file_rows;
            file_rows.push_back(
                hbox({
                    text("  Filename") | bold | size(WIDTH, EQUAL, 30) | color(accent()),
                    text("Size") | bold | size(WIDTH, EQUAL, 12) | color(accent()),
                    text("Uploaded") | bold | size(WIDTH, EQUAL, 22) | color(accent()),
                })
            );
            file_rows.push_back(separator() | color(dim()));

            if (file_list.empty())
            {
                file_rows.push_back(
                    text("  No files found") | color(dim()) | hcenter
                );
            }
            else
            {
                for (const auto& f : file_list)
                {
                    std::string size_str;
                    if (f.size < 1024)
                    {
                        size_str = std::to_string(f.size) + " B";
                    }
                    else if (f.size < 1024 * 1024)
                    {
                        size_str = std::to_string(f.size / 1024) + " KB";
                    }
                    else
                    {
                        size_str = std::to_string(f.size / (1024 * 1024)) + " MB";
                    }

                    file_rows.push_back(
                        hbox({
                            text("  " + f.filename) | size(WIDTH, EQUAL, 30),
                            text(size_str) | size(WIDTH, EQUAL, 12),
                            text(f.uploaded_at) | size(WIDTH, EQUAL, 22),
                        })
                    );
                }
            }

            auto status_element = text("");
            if (!status_message.empty())
            {
                status_element = text(status_message)
                    | color(status_is_error ? error_c() : success_c())
                    | hcenter;
            }

            return vbox({
                filler(),
                hbox({
                    filler(),
                    styled_box("📋 Your Files (" + std::to_string(file_list.size()) + ")", vbox({
                        vbox(file_rows) | size(WIDTH, GREATER_THAN, 60),
                        text("") | size(HEIGHT, EQUAL, 1),
                        hbox({
                            files_refresh->Render(),
                            text("  "),
                            files_back->Render(),
                        }) | hcenter,
                        text("") | size(HEIGHT, EQUAL, 1),
                        status_element,
                    })),
                    filler(),
                }),
                filler(),
            });
        });

        // ── Main Application Router ─────────────────────────────────────────
        auto main_container = Container::Tab({
            login_renderer,
            dashboard_renderer,
            upload_renderer,
            download_renderer,
            files_renderer,
        }, &current_screen_index);

        screen.Loop(main_container);
    }

} // namespace vault::client