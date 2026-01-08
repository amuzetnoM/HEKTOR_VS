#pragma once

#include <string>

namespace vdb::cli {

/**
 * Terminal color and formatting utilities
 */
class Colors {
public:
    // Check if terminal supports colors
    static bool supports_colors();
    
    // Enable/disable colors
    static void enable(bool enabled = true);
    static bool is_enabled();

    // Text colors
    static std::string red(const std::string& text);
    static std::string green(const std::string& text);
    static std::string yellow(const std::string& text);
    static std::string blue(const std::string& text);
    static std::string magenta(const std::string& text);
    static std::string cyan(const std::string& text);
    static std::string white(const std::string& text);
    static std::string gray(const std::string& text);

    // Text styles
    static std::string bold(const std::string& text);
    static std::string dim(const std::string& text);
    static std::string italic(const std::string& text);
    static std::string underline(const std::string& text);

    // Semantic colors
    static std::string error(const std::string& text);
    static std::string warning(const std::string& text);
    static std::string success(const std::string& text);
    static std::string info(const std::string& text);
    static std::string highlight(const std::string& text);

    // Reset
    static std::string reset(const std::string& text);

private:
    static bool enabled_;
    static std::string apply(const std::string& text, const std::string& code);
};

// ANSI color codes
namespace ansi {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    constexpr const char* ITALIC = "\033[3m";
    constexpr const char* UNDERLINE = "\033[4m";
    
    constexpr const char* BLACK = "\033[30m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
    constexpr const char* GRAY = "\033[90m";
    
    constexpr const char* BG_BLACK = "\033[40m";
    constexpr const char* BG_RED = "\033[41m";
    constexpr const char* BG_GREEN = "\033[42m";
    constexpr const char* BG_YELLOW = "\033[43m";
    constexpr const char* BG_BLUE = "\033[44m";
    constexpr const char* BG_MAGENTA = "\033[45m";
    constexpr const char* BG_CYAN = "\033[46m";
    constexpr const char* BG_WHITE = "\033[47m";
}

} // namespace vdb::cli
