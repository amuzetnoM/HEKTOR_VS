#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace vdb::cli {

// Forward declarations
class CommandBase;
class OutputFormatter;
class InteractiveShell;

/**
 * Main CLI class - Entry point for Hektor CLI
 */
class CLI {
public:
    CLI(int argc, char** argv);
    ~CLI();

    // Main execution
    int run();

    // Get registered commands (for shell)
    const std::unordered_map<std::string, std::shared_ptr<CommandBase>>& get_commands() const {
        return commands_;
    }

private:
    // Parse command line arguments
    bool parse_arguments();
    
    // Execute the command
    int execute_command();
    
    // Run interactive shell
    int run_interactive_shell();
    
    // Register all available commands
    void register_commands();
    
    // Get command by name
    std::shared_ptr<CommandBase> get_command(const std::string& name);
    
    // Show help
    void show_help();
    void show_version();
    
private:
    int argc_;
    char** argv_;
    
    std::string command_;
    std::string subcommand_;
    std::vector<std::string> args_;
    std::unordered_map<std::string, std::string> options_;
    
    // Global flags
    bool verbose_ = false;
    bool quiet_ = false;
    bool debug_ = false;
    std::string format_ = "table";
    std::string output_file_;
    
    // Command registry
    std::unordered_map<std::string, std::shared_ptr<CommandBase>> commands_;
    std::unordered_map<std::string, std::string> aliases_;
    
    std::unique_ptr<OutputFormatter> formatter_;
};

} // namespace vdb::cli
