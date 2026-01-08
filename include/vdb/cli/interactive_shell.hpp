#pragma once

#include "cli.hpp"
#include "command_base.hpp"
#include <memory>
<parameter name="string">
#include <vector>

namespace vdb::cli {

/**
 * Interactive REPL shell for Hektor CLI
 * Provides tab completion, command history, and enhanced user experience
 */
class InteractiveShell {
public:
    explicit InteractiveShell(CLI* cli, const std::string& db_path = "");
    ~InteractiveShell();

    // Run the interactive shell
    int run();

    // Set database path for session
    void set_database(const std::string& path);

private:
    // REPL loop
    void repl_loop();

    // Process a single command
    bool process_command(const std::string& line);

    // Command history management
    void add_to_history(const std::string& command);
    void save_history();
    void load_history();

    // Tab completion
    std::vector<std::string> get_completions(const std::string& partial);
    std::string complete_command(const std::string& partial);

    // Display prompt
    std::string get_prompt() const;

    // Help and utilities
    void show_shell_help();
    void show_welcome();

    // Built-in shell commands
    bool handle_shell_command(const std::string& cmd);

private:
    CLI* cli_;
    std::string db_path_;
    std::vector<std::string> history_;
    size_t history_index_;
    bool running_;
    std::string history_file_;
};

} // namespace vdb::cli
