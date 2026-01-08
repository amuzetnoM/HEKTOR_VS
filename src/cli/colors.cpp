#include "vdb/cli/colors.hpp"
#include <cstdlib>
#include <unistd.h>

namespace vdb::cli {

bool Colors::enabled_ = true;

bool Colors::supports_colors() {
    // Check if stdout is a terminal
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }
    
    // Check TERM environment variable
    const char* term = std::getenv("TERM");
    if (!term) {
        return false;
    }
    
    std::string term_str(term);
    return term_str != "dumb" && term_str != "unknown";
}

void Colors::enable(bool enabled) {
    enabled_ = enabled && supports_colors();
}

bool Colors::is_enabled() {
    return enabled_;
}

std::string Colors::apply(const std::string& text, const std::string& code) {
    if (!enabled_) {
        return text;
    }
    return code + text + ansi::RESET;
}

std::string Colors::red(const std::string& text) {
    return apply(text, ansi::RED);
}

std::string Colors::green(const std::string& text) {
    return apply(text, ansi::GREEN);
}

std::string Colors::yellow(const std::string& text) {
    return apply(text, ansi::YELLOW);
}

std::string Colors::blue(const std::string& text) {
    return apply(text, ansi::BLUE);
}

std::string Colors::magenta(const std::string& text) {
    return apply(text, ansi::MAGENTA);
}

std::string Colors::cyan(const std::string& text) {
    return apply(text, ansi::CYAN);
}

std::string Colors::white(const std::string& text) {
    return apply(text, ansi::WHITE);
}

std::string Colors::gray(const std::string& text) {
    return apply(text, ansi::GRAY);
}

std::string Colors::bold(const std::string& text) {
    return apply(text, ansi::BOLD);
}

std::string Colors::dim(const std::string& text) {
    return apply(text, ansi::DIM);
}

std::string Colors::italic(const std::string& text) {
    return apply(text, ansi::ITALIC);
}

std::string Colors::underline(const std::string& text) {
    return apply(text, ansi::UNDERLINE);
}

std::string Colors::error(const std::string& text) {
    return apply("✗ " + text, std::string(ansi::BOLD) + ansi::RED);
}

std::string Colors::warning(const std::string& text) {
    return apply("⚠ " + text, std::string(ansi::BOLD) + ansi::YELLOW);
}

std::string Colors::success(const std::string& text) {
    return apply("✓ " + text, std::string(ansi::BOLD) + ansi::GREEN);
}

std::string Colors::info(const std::string& text) {
    return apply("ℹ " + text, ansi::CYAN);
}

std::string Colors::highlight(const std::string& text) {
    return apply(text, std::string(ansi::BOLD) + ansi::CYAN);
}

std::string Colors::reset(const std::string& text) {
    return text + ansi::RESET;
}

} // namespace vdb::cli
