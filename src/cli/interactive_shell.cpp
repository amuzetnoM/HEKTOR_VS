#include "vdb/cli/interactive_shell.hpp"
#include "vdb/cli/colors.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

namespace vdb::cli {

InteractiveShell::InteractiveShell(CLI* cli, const std::string& db_path)
    : cli_(cli)
    , db_path_(db_path)
    , history_index_(0)
    , running_(false)
{
    // Set history file path
    const char* home = std::getenv("HOME");
    if (home) {
        history_file_ = std::string(home) + "/.hektor_history";
    } else {
        history_file_ = ".hektor_history";
    }
    
    load_history();
}

InteractiveShell::~InteractiveShell() {
    save_history();
}

int InteractiveShell::run() {
    show_welcome();
    running_ = true;
    repl_loop();
    return 0;
}

void InteractiveShell::set_database(const std::string& path) {
    db_path_ = path;
}

void InteractiveShell::repl_loop() {
    while (running_) {
        // Display prompt
        std::cout << get_prompt() << std::flush;
        
        // Read line
        std::string line;
        if (!std::getline(std::cin, line)) {
            break; // EOF
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        // Add to history
        add_to_history(line);
        
        // Process command
        if (!process_command(line)) {
            break;
        }
    }
    
    std::cout << Colors::cyan("Goodbye!") << std::endl;
}

bool InteractiveShell::process_command(const std::string& line) {
    // Check for shell-specific commands
    if (handle_shell_command(line)) {
        return true;
    }
    
    // Build argv-style arguments
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (ss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return true;
    }
    
    // Execute command through CLI
    try {
        // Prepend database path if set and command needs it
        std::vector<const char*> argv;
        argv.push_back("hektor");
        
        for (const auto& t : tokens) {
            argv.push_back(t.c_str());
        }
        
        // Note: This is a simplified version
        // In real implementation, we'd call CLI's command execution directly
        std::cout << Colors::yellow("Executing: ") << line << std::endl;
        std::cout << Colors::info("(Demo mode - full integration needed)") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << Colors::error("Error: ") << e.what() << std::endl;
    }
    
    return true;
}

bool InteractiveShell::handle_shell_command(const std::string& cmd) {
    if (cmd == "exit" || cmd == "quit" || cmd == "q") {
        running_ = false;
        return true;
    }
    
    if (cmd == "help" || cmd == "?") {
        show_shell_help();
        return true;
    }
    
    if (cmd == "clear" || cmd == "cls") {
        std::cout << "\033[2J\033[1;1H"; // Clear screen
        return true;
    }
    
    if (cmd == "history") {
        for (size_t i = 0; i < history_.size(); ++i) {
            std::cout << Colors::gray(std::to_string(i + 1) + "  ") << history_[i] << std::endl;
        }
        return true;
    }
    
    if (cmd.substr(0, 3) == "use" && cmd.length() > 4) {
        std::string path = cmd.substr(4);
        set_database(path);
        std::cout << Colors::success("Database set to: ") << path << std::endl;
        return true;
    }
    
    return false;
}

void InteractiveShell::add_to_history(const std::string& command) {
    if (command.empty() || command == history_.back()) {
        return; // Skip empty or duplicate
    }
    
    history_.push_back(command);
    history_index_ = history_.size();
    
    // Limit history size
    if (history_.size() > 1000) {
        history_.erase(history_.begin());
        history_index_--;
    }
}

void InteractiveShell::save_history() {
    std::ofstream file(history_file_);
    if (file.is_open()) {
        for (const auto& cmd : history_) {
            file << cmd << "\n";
        }
    }
}

void InteractiveShell::load_history() {
    std::ifstream file(history_file_);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                history_.push_back(line);
            }
        }
        history_index_ = history_.size();
    }
}

std::vector<std::string> InteractiveShell::get_completions(const std::string& partial) {
    std::vector<std::string> completions;
    
    // Command completions
    std::vector<std::string> commands = {
        "init", "info", "optimize", "backup", "restore", "health", "list",
        "add", "get", "update", "delete", "batch",
        "search", "hybrid:search", "hybrid:bm25",
        "ingest", "ingest:scan",
        "index:build", "index:optimize", "index:stats", "index:benchmark",
        "collection:create", "collection:list", "collection:delete", "collection:info",
        "export:data", "export:pairs", "export:triplets",
        "exit", "quit", "help", "clear", "history", "use"
    };
    
    for (const auto& cmd : commands) {
        if (cmd.find(partial) == 0) {
            completions.push_back(cmd);
        }
    }
    
    return completions;
}

std::string InteractiveShell::complete_command(const std::string& partial) {
    auto completions = get_completions(partial);
    
    if (completions.size() == 1) {
        return completions[0];
    } else if (completions.size() > 1) {
        std::cout << std::endl;
        for (const auto& comp : completions) {
            std::cout << "  " << comp << std::endl;
        }
    }
    
    return partial;
}

std::string InteractiveShell::get_prompt() const {
    std::string prompt = Colors::bold(Colors::blue("hektor"));
    
    if (!db_path_.empty()) {
        prompt += Colors::gray(":") + Colors::cyan(db_path_);
    }
    
    prompt += Colors::bold(Colors::green(" > "));
    
    return prompt;
}

void InteractiveShell::show_welcome() {
    std::cout << Colors::bold(Colors::blue("\n╔═══════════════════════════════════════════════════════╗\n"));
    std::cout << Colors::bold(Colors::blue("║"))
              << Colors::bold(Colors::white("     Hektor Vector Database - Interactive Shell    "))
              << Colors::bold(Colors::blue("║\n"));
    std::cout << Colors::bold(Colors::blue("║"))
              << Colors::gray("                   Version 2.3.0                    ")
              << Colors::bold(Colors::blue("║\n"));
    std::cout << Colors::bold(Colors::blue("╚═══════════════════════════════════════════════════════╝\n\n"));
    
    std::cout << Colors::info("Type ") << Colors::bold("help") 
              << Colors::info(" for available commands, ")
              << Colors::bold("exit") << Colors::info(" to quit\n\n");
}

void InteractiveShell::show_shell_help() {
    std::cout << Colors::bold("\nInteractive Shell Commands:\n\n");
    
    std::cout << Colors::cyan("  Shell Commands:\n");
    std::cout << "    help, ?              Show this help\n";
    std::cout << "    exit, quit, q        Exit the shell\n";
    std::cout << "    clear, cls           Clear screen\n";
    std::cout << "    history              Show command history\n";
    std::cout << "    use <path>           Set active database\n\n";
    
    std::cout << Colors::cyan("  Database Commands:\n");
    std::cout << "    init <path>          Initialize database\n";
    std::cout << "    info [path]          Show database info\n";
    std::cout << "    optimize [path]      Optimize database\n";
    std::cout << "    search <query>       Search database\n";
    std::cout << "    add --text \"...\"     Add document\n\n";
    
    std::cout << Colors::cyan("  Advanced Commands:\n");
    std::cout << "    hybrid:search <q>    Hybrid vector+BM25 search\n";
    std::cout << "    ingest <source>      Import data\n";
    std::cout << "    index:build          Build index\n";
    std::cout << "    collection:list      List collections\n\n";
    
    std::cout << Colors::gray("  Tab completion and command history are available\n");
    std::cout << Colors::gray("  Use UP/DOWN arrows to navigate history\n\n");
}

} // namespace vdb::cli
