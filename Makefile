# Cross-platform Makefile for VaultCLI

CXX = clang++
CXXFLAGS = -std=c++20 -O2 -march=native -DNDEBUG
BUILD_DIR = build

# Detect OS and set color codes properly
ifeq ($(OS),Windows_NT)
    DETECTED_OS = Windows
    NINJA = ninja.exe
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = mkdir
    CD = cd
    # No colors on Windows by default
    COLOR_GREEN =
    COLOR_RED =
    COLOR_YELLOW =
    COLOR_RESET =
else
    DETECTED_OS = $(shell uname -s)
    NINJA = ninja
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CD = cd
    # Use shell printf for colors (more reliable)
    COLOR_GREEN = \033[0;32m
    COLOR_RED = \033[0;31m
    COLOR_YELLOW = \033[1;33m
    COLOR_RESET = \033[0m
endif

.PHONY: all clean release debug help

all: release

# Release build
release:
	@printf "$(COLOR_GREEN)🚀 Building VaultCLI (Release Mode)$(COLOR_RESET)\n"
	@$(RMDIR) $(BUILD_DIR) 2>/dev/null || true
	@$(MKDIR) $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
		cmake -G Ninja \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_CXX_FLAGS="-O2 -march=native -DNDEBUG" \
			.. && \
		$(NINJA)
	@printf "$(COLOR_GREEN)✅ Build successful! Binary in $(BUILD_DIR)/$(COLOR_RESET)\n"

# Debug build
debug:
	@printf "$(COLOR_YELLOW)🐛 Building VaultCLI (Debug Mode)$(COLOR_RESET)\n"
	@$(RMDIR) $(BUILD_DIR) 2>/dev/null || true
	@$(MKDIR) $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
		cmake -G Ninja \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_CXX_FLAGS="-g -O0 -DDEBUG" \
			.. && \
		$(NINJA)
	@printf "$(COLOR_GREEN)✅ Build successful! Binary in $(BUILD_DIR)/$(COLOR_RESET)\n"

# Clean
clean:
	@printf "$(COLOR_YELLOW)🧹 Cleaning build directory...$(COLOR_RESET)\n"
	@$(RMDIR) $(BUILD_DIR) 2>/dev/null || true
	@printf "$(COLOR_GREEN)✅ Clean complete$(COLOR_RESET)\n"

# Help
help:
	@printf "Available commands:\n"
	@printf "  make release  - Build release version (optimized, -O2)\n"
	@printf "  make debug    - Build debug version (with symbols)\n"
	@printf "  make clean    - Remove build directory\n"
	@printf "  make help     - Show this help message\n"

.DEFAULT_GOAL := release