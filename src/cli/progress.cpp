#include "vdb/cli/progress.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace vdb::cli {

ProgressBar::ProgressBar(size_t total, const std::string& description)
    : total_(total)
    , current_(0)
    , description_(description)
    , start_time_(std::chrono::steady_clock::now())
    , finished_(false)
    , width_(50)
{
}

ProgressBar::~ProgressBar() {
    if (!finished_) {
        finish();
    }
}

void ProgressBar::update(size_t current) {
    current_ = current;
    render();
}

void ProgressBar::increment() {
    current_++;
    if (current_ % 10 == 0 || current_ == total_) {
        render();
    }
}

void ProgressBar::finish() {
    if (!finished_) {
        current_ = total_;
        render();
        std::cout << std::endl;
        finished_ = true;
    }
}

void ProgressBar::set_description(const std::string& desc) {
    description_ = desc;
}

void ProgressBar::render() {
    if (total_ == 0) return;
    
    double progress = static_cast<double>(current_) / total_;
    int pos = static_cast<int>(width_ * progress);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    
    double rate = current_ > 0 ? static_cast<double>(current_) / elapsed : 0.0;
    double eta = rate > 0 ? (total_ - current_) / rate : 0.0;
    
    std::cout << "\r" << description_ << " [";
    
    for (int i = 0; i < width_; ++i) {
        if (i < pos) {
            std::cout << "█";
        } else if (i == pos) {
            std::cout << "▓";
        } else {
            std::cout << "░";
        }
    }
    
    std::cout << "] " << std::fixed << std::setprecision(1) << (progress * 100.0) << "% ";
    std::cout << current_ << "/" << total_;
    
    if (rate > 0) {
        std::cout << " (" << std::fixed << std::setprecision(1) << rate << " items/s";
        if (eta > 0) {
            std::cout << ", ETA: " << format_time(eta);
        }
        std::cout << ")";
    }
    
    std::cout << std::flush;
}

std::string ProgressBar::format_time(double seconds) const {
    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds - hours * 3600) / 60);
    int secs = static_cast<int>(seconds - hours * 3600 - minutes * 60);
    
    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << minutes << "m";
    } else if (minutes > 0) {
        oss << minutes << "m " << secs << "s";
    } else {
        oss << secs << "s";
    }
    return oss.str();
}

std::string ProgressBar::format_size(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

// Spinner implementation
Spinner::Spinner(const std::string& message)
    : message_(message)
    , running_(false)
    , frame_(0)
{
}

Spinner::~Spinner() {
    stop();
}

void Spinner::start() {
    running_ = true;
    last_update_ = std::chrono::steady_clock::now();
    spin();
}

void Spinner::stop() {
    if (running_) {
        running_ = false;
        std::cout << "\r" << std::string(message_.length() + 10, ' ') << "\r" << std::flush;
    }
}

void Spinner::set_message(const std::string& msg) {
    message_ = msg;
}

void Spinner::spin() {
    const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    const size_t num_frames = 10;
    
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_update_
        ).count();
        
        if (elapsed >= 80) { // Update every 80ms
            std::cout << "\r" << frames[frame_] << " " << message_ << std::flush;
            frame_ = (frame_ + 1) % num_frames;
            last_update_ = now;
        }
    }
}

} // namespace vdb::cli
