cmake_minimum_required(VERSION 3.20)
project(VaultCLI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SPEED: Persistent download cache (cross-platform)
if(NOT FETCHCONTENT_BASE_DIR)
    if(WIN32)
        set(FETCHCONTENT_BASE_DIR "$ENV{USERPROFILE}/.cache/vaultcli/fetchcontent" CACHE PATH "Directory to keep downloaded dependencies")
    else()
        set(FETCHCONTENT_BASE_DIR "$ENV{HOME}/.cache/vaultcli/fetchcontent" CACHE PATH "Directory to keep downloaded dependencies")
    endif()
endif()
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable network updates" FORCE)

include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(httplib nlohmann_json)

# Find system FTXUI - lowercase with COMPONENTS
find_package(ftxui QUIET COMPONENTS component screen dom)
if(ftxui_FOUND)
    message(STATUS "Found system FTXUI in ${ftxui_DIR}")
else()
    message(STATUS "System FTXUI not found, downloading shallow clone...")
    FetchContent_Declare(
        ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
        GIT_TAG        v5.0.0
        GIT_SHALLOW    TRUE
    )
    
    # Disable optional components to save build time
    set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(ftxui)
endif()

# Find OpenSSL (works on both Linux and Windows)
find_package(OpenSSL REQUIRED)

# ─── Subdirectories ──────────────────────────────────────────────────────────
add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)