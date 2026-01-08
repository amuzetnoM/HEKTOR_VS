#pragma once

#include <string>
#include <chrono>
#include <atomic>

namespace vdb::cli {

/**
 * Progress indicator for long-running operations
 */
class ProgressBar {
public:
    explicit ProgressBar(size_t total, const std::string& description = "");
    ~ProgressBar();

    // Update progress
    void update(size_t current);
    void increment();
    
    // Finish progress
    void finish();
    
    // Set description
    void set_description(const std::string& desc);

private:
    void render();
    std::string format_time(double seconds) const;
    std::string format_size(size_t bytes) const;

private:
    size_t total_;
    std::atomic<size_t> current_;
    std::string description_;
    std::chrono::steady_clock::time_point start_time_;
    bool finished_;
    int width_;
};

/**
 * Simple spinner for indeterminate operations
 */
class Spinner {
public:
    explicit Spinner(const std::string& message = "Processing...");
    ~Spinner();

    void start();
    void stop();
    void set_message(const std::string& msg);

private:
    void spin();

private:
    std::string message_;
    std::atomic<bool> running_;
    size_t frame_;
    std::chrono::steady_clock::time_point last_update_;
};

} // namespace vdb::cli
