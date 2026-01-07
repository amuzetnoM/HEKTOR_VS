#include "vdb/cli/cli.hpp"
#include "vdb/cli/command_base.hpp"
#include "vdb/cli/output_formatter.hpp"
#include "vdb/cli/commands/db_commands.hpp"
#include "vdb/cli/commands/data_commands.hpp"
#include "vdb/cli/commands/search_commands.hpp"
#include <iostream>
#include <algorithm>

namespace vdb::cli {

CLI::CLI(int argc, char** argv) 
    : argc_(argc), argv_(argv) {
    formatter_ = std::make_unique<OutputFormatter>();
    register_commands();
}

CLI::~CLI() = default;

int CLI::run() {
    if (argc_ < 2) {
        show_help();
        return 1;
    }
    
    if (!parse_arguments()) {
        return 1;
    }
    
    // Handle help and version
    if (command_ == "help" || command_ == "--help" || command_ == "-h") {
        show_help();
        return 0;
    }
    
    if (command_ == "version" || command_ == "--version" || command_ == "-V") {
        show_version();
        return 0;
    }
    
    return execute_command();
}

bool CLI::parse_arguments() {
    int i = 1;
    
    // Parse global options first
    while (i < argc_ && argv_[i][0] == '-') {
        std::string arg = argv_[i];
        
        if (arg == "-v" || arg == "--verbose") {
            verbose_ = true;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet_ = true;
        } else if (arg == "-d" || arg == "--debug") {
            debug_ = true;
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < argc_) {
                format_ = argv_[++i];
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc_) {
                output_file_ = argv_[++i];
            }
        } else {
            break; // Not a global option
        }
        i++;
    }
    
    // Get command
    if (i < argc_) {
        command_ = argv_[i++];
    } else {
        std::cerr << "Error: No command specified\n";
        return false;
    }
    
    // Parse remaining arguments and options
    while (i < argc_) {
        std::string arg = argv_[i];
        
        if (arg[0] == '-' && arg.size() > 1) {
            // It's an option
            std::string key = arg;
            std::string value;
            
            // Check if next arg is a value
            if (i + 1 < argc_ && argv_[i + 1][0] != '-') {
                value = argv_[i + 1];
                i += 2;
            } else {
                // Flag without value
                value = "true";
                i++;
            }
            
            options_[key] = value;
        } else {
            // It's a positional argument
            args_.push_back(arg);
            i++;
        }
    }
    
    return true;
}

int CLI::execute_command() {
    auto cmd = get_command(command_);
    
    if (!cmd) {
        std::cerr << "Error: Unknown command '" << command_ << "'\n";
        std::cerr << "Run 'hektor help' for available commands\n";
        return 1;
    }
    
    try {
        return cmd->execute(args_, options_);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

void CLI::register_commands() {
    // Database commands
    auto db_init = std::make_shared<DbInitCommand>();
    commands_[db_init->name()] = db_init;
    
    auto db_info = std::make_shared<DbInfoCommand>();
    commands_[db_info->name()] = db_info;
    aliases_["info"] = db_info->name();
    
    // Data commands
    auto data_add = std::make_shared<DataAddCommand>();
    commands_[data_add->name()] = data_add;
    aliases_["add"] = data_add->name();
    
    auto data_get = std::make_shared<DataGetCommand>();
    commands_[data_get->name()] = data_get;
    aliases_["get"] = data_get->name();
    
    auto data_delete = std::make_shared<DataDeleteCommand>();
    commands_[data_delete->name()] = data_delete;
    aliases_["delete"] = data_delete->name();
    aliases_["rm"] = data_delete->name();
    
    // Search command
    auto search = std::make_shared<SearchCommand>();
    commands_[search->name()] = search;
    aliases_["s"] = search->name();
}

std::shared_ptr<CommandBase> CLI::get_command(const std::string& name) {
    // Try direct lookup
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return it->second;
    }
    
    // Try alias lookup
    auto alias_it = aliases_.find(name);
    if (alias_it != aliases_.end()) {
        return commands_[alias_it->second];
    }
    
    return nullptr;
}

void CLI::show_help() {
    std::cout << R"(
Hektor - High-Performance Vector Database CLI
Version 2.3.0

Usage: hektor [OPTIONS] <COMMAND> [ARGS]

Global Options:
  -v, --verbose         Verbose output
  -q, --quiet           Quiet mode
  -d, --debug           Debug mode
  -f, --format FORMAT   Output format (table|json|csv)
  -o, --output FILE     Write output to file
  -h, --help            Show help
      --version         Show version

Commands:
  Database Management:
    init <path>           Initialize a new database
    info <path>           Show database information
  
  Data Operations:
    add <db>              Add a document
    get <db> <id>         Get document by ID
    delete <db> <id>      Delete a document
  
  Search:
    search <db> <query>   Search the database
  
  General:
    help                  Show this help message
    version               Show version information

Examples:
  # Initialize database
  hektor init ./mydb

  # Add a document
  hektor add ./mydb --text "Gold prices rising"

  # Search
  hektor search ./mydb "gold outlook" -k 10

  # Get database info
  hektor info ./mydb

For more information, visit: https://github.com/amuzetnoM/hektor
)";
}

void CLI::show_version() {
    std::cout << "Hektor CLI version 2.3.0\n";
    std::cout << "Vector Database Engine\n";
}

} // namespace vdb::cli
