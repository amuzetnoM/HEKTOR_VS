---
title: "CLI Implementation Blueprint"
description: "Technical implementation plan for Hektor CLI"
version: "3.0.0"
last_updated: "2026-01-07"
status: "Design"
---

# Hektor CLI - Implementation Blueprint

## Architecture Overview

### Component Structure

```
src/cli/
├── main.cpp                    # Entry point
├── cli.hpp                     # Main CLI class
├── cli.cpp                     # CLI implementation
│
├── parser/
│   ├── argument_parser.hpp     # Argument parsing
│   ├── argument_parser.cpp
│   ├── command_parser.hpp      # Command parsing
│   └── command_parser.cpp
│
├── commands/
│   ├── command_base.hpp        # Base command interface
│   ├── command_base.cpp
│   ├── db_commands.hpp         # Database commands
│   ├── db_commands.cpp
│   ├── data_commands.hpp       # Data operations
│   ├── data_commands.cpp
│   ├── search_commands.hpp     # Search commands
│   ├── search_commands.cpp
│   ├── hybrid_commands.hpp     # Hybrid search
│   ├── hybrid_commands.cpp
│   ├── ingest_commands.hpp     # Ingestion
│   ├── ingest_commands.cpp
│   ├── index_commands.hpp      # Index management
│   ├── index_commands.cpp
│   ├── collection_commands.hpp # Collections
│   ├── collection_commands.cpp
│   ├── meta_commands.hpp       # Metadata
│   ├── meta_commands.cpp
│   ├── export_commands.hpp     # Export/Import
│   ├── export_commands.cpp
│   ├── model_commands.hpp      # Model management
│   ├── model_commands.cpp
│   ├── cluster_commands.hpp    # Distributed
│   ├── cluster_commands.cpp
│   ├── monitor_commands.hpp    # Monitoring
│   ├── monitor_commands.cpp
│   ├── report_commands.hpp     # Reports
│   ├── report_commands.cpp
│   ├── auth_commands.hpp       # Security
│   ├── auth_commands.cpp
│   ├── config_commands.hpp     # Configuration
│   ├── config_commands.cpp
│   ├── rag_commands.hpp        # RAG operations
│   ├── rag_commands.cpp
│   ├── quantize_commands.hpp   # Quantization
│   ├── quantize_commands.cpp
│   └── util_commands.hpp       # Utilities
│       └── util_commands.cpp
│
├── output/
│   ├── formatter.hpp           # Output formatting
│   ├── formatter.cpp
│   ├── json_formatter.hpp      # JSON format
│   ├── json_formatter.cpp
│   ├── table_formatter.hpp     # Table format
│   ├── table_formatter.cpp
│   ├── csv_formatter.hpp       # CSV format
│   ├── csv_formatter.cpp
│   └── yaml_formatter.hpp      # YAML format
│       └── yaml_formatter.cpp
│
├── interactive/
│   ├── shell.hpp               # Interactive shell
│   ├── shell.cpp
│   ├── repl.hpp                # REPL loop
│   ├── repl.cpp
│   ├── completion.hpp          # Tab completion
│   ├── completion.cpp
│   └── history.hpp             # Command history
│       └── history.cpp
│
├── config/
│   ├── config_manager.hpp      # Config management
│   ├── config_manager.cpp
│   ├── profile.hpp             # Profile management
│   └── profile.cpp
│
└── utils/
    ├── colors.hpp              # Terminal colors
    ├── colors.cpp
    ├── progress.hpp            # Progress bars
    ├── progress.cpp
    ├── validators.hpp          # Input validation
    ├── validators.cpp
    └── helpers.hpp             # Helper functions
        └── helpers.cpp
```

## Core Classes

### 1. CLI Main Class

```cpp
// cli.hpp
#pragma once

#include "parser/command_parser.hpp"
#include "commands/command_base.hpp"
#include "output/formatter.hpp"
#include "config/config_manager.hpp"
#include <memory>
#include <string>
#include <vector>

namespace vdb::cli {

class CLI {
public:
    CLI(int argc, char** argv);
    ~CLI();

    // Main execution
    int run();

private:
    // Parse command line
    Result<ParsedCommand> parse_arguments();
    
    // Execute command
    int execute_command(const ParsedCommand& cmd);
    
    // Register all commands
    void register_commands();
    
    // Get command by name
    std::shared_ptr<CommandBase> get_command(const std::string& name);
    
    // Error handling
    void handle_error(const Error& error);
    
    // Help system
    void show_help(const std::string& command = "");
    void show_version();
    
private:
    int argc_;
    char** argv_;
    std::unique_ptr<CommandParser> parser_;
    std::unique_ptr<ConfigManager> config_;
    std::unique_ptr<OutputFormatter> formatter_;
    
    // Command registry
    std::unordered_map<std::string, std::shared_ptr<CommandBase>> commands_;
    std::unordered_map<std::string, std::string> aliases_;
};

} // namespace vdb::cli
```

### 2. Command Base Interface

```cpp
// commands/command_base.hpp
#pragma once

#include "vdb/core.hpp"
#include "../output/formatter.hpp"
#include <string>
#include <vector>

namespace vdb::cli {

struct CommandContext {
    std::string db_path;
    std::shared_ptr<ConfigManager> config;
    std::shared_ptr<OutputFormatter> formatter;
    bool verbose = false;
    bool quiet = false;
    bool debug = false;
};

class CommandBase {
public:
    virtual ~CommandBase() = default;
    
    // Command metadata
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual std::vector<std::string> aliases() const { return {}; }
    
    // Help text
    virtual std::string usage() const = 0;
    virtual std::string help() const = 0;
    
    // Execution
    virtual Result<void> execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options,
        const CommandContext& context
    ) = 0;
    
    // Validation
    virtual Result<void> validate(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) const;
    
    // Tab completion
    virtual std::vector<std::string> complete(
        const std::vector<std::string>& args,
        size_t cursor_pos
    ) const;
};

} // namespace vdb::cli
```

### 3. Argument Parser

```cpp
// parser/argument_parser.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace vdb::cli {

struct ParsedArgs {
    std::string command;
    std::string subcommand;
    std::vector<std::string> positional;
    std::unordered_map<std::string, std::string> options;
    std::unordered_map<std::string, bool> flags;
};

class ArgumentParser {
public:
    ArgumentParser();
    
    // Parse arguments
    Result<ParsedArgs> parse(int argc, char** argv);
    
    // Define expected options
    void add_option(
        const std::string& name,
        const std::string& short_name,
        const std::string& description,
        bool required = false,
        const std::string& default_value = ""
    );
    
    void add_flag(
        const std::string& name,
        const std::string& short_name,
        const std::string& description
    );
    
    void add_positional(
        const std::string& name,
        const std::string& description,
        bool required = true
    );
    
    // Generate help
    std::string generate_help() const;

private:
    struct Option {
        std::string name;
        std::string short_name;
        std::string description;
        bool required;
        std::string default_value;
    };
    
    struct Flag {
        std::string name;
        std::string short_name;
        std::string description;
    };
    
    struct Positional {
        std::string name;
        std::string description;
        bool required;
    };
    
    std::vector<Option> options_;
    std::vector<Flag> flags_;
    std::vector<Positional> positionals_;
};

} // namespace vdb::cli
```

### 4. Output Formatter

```cpp
// output/formatter.hpp
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace vdb::cli {

enum class OutputFormat {
    Table,
    JSON,
    CSV,
    YAML,
    Compact
};

class OutputFormatter {
public:
    explicit OutputFormatter(OutputFormat format = OutputFormat::Table);
    
    // Format single value
    std::string format(const nlohmann::json& data) const;
    
    // Format list
    std::string format_list(const std::vector<nlohmann::json>& items) const;
    
    // Format table
    std::string format_table(
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    ) const;
    
    // Format error
    std::string format_error(const Error& error) const;
    
    // Format success message
    std::string format_success(const std::string& message) const;
    
    // Set format
    void set_format(OutputFormat format);
    
    // Color support
    void set_color_enabled(bool enabled);

private:
    OutputFormat format_;
    bool color_enabled_;
};

} // namespace vdb::cli
```

### 5. Configuration Manager

```cpp
// config/config_manager.hpp
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <filesystem>

namespace vdb::cli {

class ConfigManager {
public:
    ConfigManager();
    
    // Load configuration
    Result<void> load();
    Result<void> load_from_file(const std::filesystem::path& path);
    
    // Save configuration
    Result<void> save();
    Result<void> save_to_file(const std::filesystem::path& path);
    
    // Get/Set values
    std::optional<std::string> get(const std::string& key) const;
    void set(const std::string& key, const std::string& value);
    void unset(const std::string& key);
    
    // Profile management
    Result<void> create_profile(const std::string& name);
    Result<void> use_profile(const std::string& name);
    std::vector<std::string> list_profiles() const;
    std::string current_profile() const;
    
    // Config paths
    static std::filesystem::path global_config_path();
    static std::filesystem::path local_config_path();

private:
    nlohmann::json config_;
    std::filesystem::path config_path_;
    std::string current_profile_;
};

} // namespace vdb::cli
```

## Command Implementations

### Database Commands

```cpp
// commands/db_commands.hpp
#pragma once

#include "command_base.hpp"

namespace vdb::cli {

class DbInitCommand : public CommandBase {
public:
    std::string name() const override { return "init"; }
    std::string description() const override { 
        return "Initialize a new database"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"create", "new"}; 
    }
    
    std::string usage() const override {
        return "hektor db init <path> [OPTIONS]";
    }
    
    std::string help() const override;
    
    Result<void> execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options,
        const CommandContext& context
    ) override;
};

class DbInfoCommand : public CommandBase {
public:
    std::string name() const override { return "info"; }
    std::string description() const override { 
        return "Show database information"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"stats", "status"}; 
    }
    
    std::string usage() const override {
        return "hektor db info <path>";
    }
    
    std::string help() const override;
    
    Result<void> execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options,
        const CommandContext& context
    ) override;
};

class DbOptimizeCommand : public CommandBase {
public:
    std::string name() const override { return "optimize"; }
    std::string description() const override { 
        return "Optimize database"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"compact"}; 
    }
    
    std::string usage() const override {
        return "hektor db optimize <path>";
    }
    
    std::string help() const override;
    
    Result<void> execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options,
        const CommandContext& context
    ) override;
};

// ... other DB commands

} // namespace vdb::cli
```

### Search Commands

```cpp
// commands/search_commands.hpp
#pragma once

#include "command_base.hpp"

namespace vdb::cli {

class SearchCommand : public CommandBase {
public:
    std::string name() const override { return "search"; }
    std::string description() const override { 
        return "Search database with semantic query"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"s", "find"}; 
    }
    
    std::string usage() const override {
        return "hektor search <db> <query> [OPTIONS]";
    }
    
    std::string help() const override;
    
    Result<void> execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options,
        const CommandContext& context
    ) override;

private:
    Result<QueryResults> perform_search(
        VectorDatabase& db,
        const std::string& query,
        const QueryOptions& opts
    );
    
    void print_results(
        const QueryResults& results,
        const OutputFormatter& formatter
    );
};

class QueryCommand : public CommandBase {
public:
    std::string name() const override { return "query"; }
    std::string description() const override { 
        return "Advanced query with filters"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"q"}; 
    }
    
    std::string usage() const override {
        return "hektor query <db> [OPTIONS]";
    }
    
    std::string help() const override;
    
    Result<void> execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options,
        const CommandContext& context
    ) override;
};

} // namespace vdb::cli
```

## Interactive Shell

### Shell Implementation

```cpp
// interactive/shell.hpp
#pragma once

#include "../commands/command_base.hpp"
#include "../output/formatter.hpp"
#include <memory>
#include <string>

namespace vdb::cli {

class InteractiveShell {
public:
    explicit InteractiveShell(const std::string& db_path);
    ~InteractiveShell();
    
    // Run the shell
    int run();

private:
    // REPL loop
    void repl_loop();
    
    // Process command
    Result<void> process_command(const std::string& line);
    
    // Command history
    void add_to_history(const std::string& line);
    std::vector<std::string> get_history() const;
    
    // Tab completion
    std::vector<std::string> complete(
        const std::string& prefix,
        size_t cursor_pos
    );
    
    // Special commands
    void cmd_help();
    void cmd_history();
    void cmd_clear();
    void cmd_config();
    void cmd_stats();
    
    // Prompt
    std::string generate_prompt() const;

private:
    std::string db_path_;
    std::unique_ptr<VectorDatabase> db_;
    std::unique_ptr<OutputFormatter> formatter_;
    std::vector<std::string> history_;
    bool running_;
};

} // namespace vdb::cli
```

## Progress Indicators

### Progress Bar Implementation

```cpp
// utils/progress.hpp
#pragma once

#include <string>
#include <chrono>

namespace vdb::cli {

class ProgressBar {
public:
    ProgressBar(size_t total, const std::string& description = "");
    
    // Update progress
    void update(size_t current);
    void increment();
    
    // Complete
    void finish();
    
    // Configure
    void set_width(size_t width);
    void set_show_percentage(bool show);
    void set_show_eta(bool show);

private:
    void render();
    std::string format_time(std::chrono::milliseconds ms) const;

private:
    size_t total_;
    size_t current_;
    std::string description_;
    size_t width_;
    bool show_percentage_;
    bool show_eta_;
    std::chrono::steady_clock::time_point start_time_;
};

// Spinner for indeterminate progress
class Spinner {
public:
    explicit Spinner(const std::string& message = "");
    ~Spinner();
    
    void start();
    void stop();
    void set_message(const std::string& message);

private:
    void spin();

private:
    std::string message_;
    bool running_;
    std::thread spinner_thread_;
};

} // namespace vdb::cli
```

## Color Support

### Terminal Colors

```cpp
// utils/colors.hpp
#pragma once

#include <string>

namespace vdb::cli::colors {

// ANSI color codes
namespace ansi {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
    
    constexpr const char* BG_RED = "\033[41m";
    constexpr const char* BG_GREEN = "\033[42m";
    constexpr const char* BG_YELLOW = "\033[43m";
}

// Helper functions
std::string colorize(const std::string& text, const std::string& color);
std::string bold(const std::string& text);
std::string dim(const std::string& text);

std::string success(const std::string& text);
std::string error(const std::string& text);
std::string warning(const std::string& text);
std::string info(const std::string& text);

// Check if colors are supported
bool colors_supported();

} // namespace vdb::cli::colors
```

## Validation

### Input Validators

```cpp
// utils/validators.hpp
#pragma once

#include <string>
#include <filesystem>

namespace vdb::cli {

class Validators {
public:
    // Path validation
    static Result<void> validate_db_path(const std::string& path);
    static Result<void> validate_file_exists(const std::string& path);
    static Result<void> validate_dir_exists(const std::string& path);
    
    // Value validation
    static Result<void> validate_positive_int(const std::string& value);
    static Result<void> validate_range(const std::string& value, int min, int max);
    static Result<void> validate_float_range(const std::string& value, float min, float max);
    
    // Format validation
    static Result<void> validate_date(const std::string& date);
    static Result<void> validate_format(const std::string& format, const std::vector<std::string>& allowed);
    
    // Database validation
    static Result<void> validate_metric(const std::string& metric);
    static Result<void> validate_dimension(int dimension);
};

} // namespace vdb::cli
```

## Build System Integration

### CMakeLists.txt Addition

```cmake
# CLI executable
add_executable(hektor
    src/cli/main.cpp
    src/cli/cli.cpp
    
    # Parser
    src/cli/parser/argument_parser.cpp
    src/cli/parser/command_parser.cpp
    
    # Commands
    src/cli/commands/command_base.cpp
    src/cli/commands/db_commands.cpp
    src/cli/commands/data_commands.cpp
    src/cli/commands/search_commands.cpp
    src/cli/commands/hybrid_commands.cpp
    src/cli/commands/ingest_commands.cpp
    src/cli/commands/index_commands.cpp
    src/cli/commands/collection_commands.cpp
    src/cli/commands/meta_commands.cpp
    src/cli/commands/export_commands.cpp
    src/cli/commands/model_commands.cpp
    src/cli/commands/cluster_commands.cpp
    src/cli/commands/monitor_commands.cpp
    src/cli/commands/report_commands.cpp
    src/cli/commands/auth_commands.cpp
    src/cli/commands/config_commands.cpp
    src/cli/commands/rag_commands.cpp
    src/cli/commands/quantize_commands.cpp
    src/cli/commands/util_commands.cpp
    
    # Output formatters
    src/cli/output/formatter.cpp
    src/cli/output/json_formatter.cpp
    src/cli/output/table_formatter.cpp
    src/cli/output/csv_formatter.cpp
    src/cli/output/yaml_formatter.cpp
    
    # Interactive
    src/cli/interactive/shell.cpp
    src/cli/interactive/repl.cpp
    src/cli/interactive/completion.cpp
    src/cli/interactive/history.cpp
    
    # Config
    src/cli/config/config_manager.cpp
    src/cli/config/profile.cpp
    
    # Utils
    src/cli/utils/colors.cpp
    src/cli/utils/progress.cpp
    src/cli/utils/validators.cpp
    src/cli/utils/helpers.cpp
)

target_link_libraries(hektor
    PRIVATE
        vdb_core
        vdb_database
        vdb_embeddings
        vdb_ingest
        vdb_hybrid
        fmt::fmt
        nlohmann_json::nlohmann_json
)

# Install
install(TARGETS hektor
    RUNTIME DESTINATION bin
)

# Shell completion scripts
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/completions/hektor.bash
    DESTINATION share/bash-completion/completions
    RENAME hektor
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/completions/hektor.zsh
    DESTINATION share/zsh/site-functions
    RENAME _hektor
)
```

## Testing Strategy

### CLI Tests

```cpp
// tests/test_cli.cpp
#include <gtest/gtest.h>
#include "cli/cli.hpp"

namespace vdb::cli::test {

class CLITest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = fs::temp_directory_path() / "test_cli_db";
        fs::create_directories(test_db_path_);
    }
    
    void TearDown() override {
        fs::remove_all(test_db_path_);
    }
    
    std::filesystem::path test_db_path_;
};

TEST_F(CLITest, InitDatabase) {
    const char* args[] = {"hektor", "db", "init", test_db_path_.c_str()};
    CLI cli(4, const_cast<char**>(args));
    
    EXPECT_EQ(cli.run(), 0);
    EXPECT_TRUE(fs::exists(test_db_path_ / "vectors.bin"));
}

TEST_F(CLITest, SearchWithFilters) {
    // Setup database first
    // ... setup code
    
    const char* args[] = {
        "hektor", "search", test_db_path_.c_str(), "test query",
        "--top", "10",
        "--type", "journal"
    };
    CLI cli(7, const_cast<char**>(args));
    
    EXPECT_EQ(cli.run(), 0);
}

// ... more tests

} // namespace vdb::cli::test
```

## Documentation Generation

### Auto-Generate Help

```cpp
// Generate help text from command definitions
std::string generate_help_text(const CommandBase& cmd) {
    std::ostringstream ss;
    
    ss << "NAME\n";
    ss << "    " << cmd.name() << " - " << cmd.description() << "\n\n";
    
    ss << "USAGE\n";
    ss << "    " << cmd.usage() << "\n\n";
    
    if (!cmd.aliases().empty()) {
        ss << "ALIASES\n";
        ss << "    ";
        for (size_t i = 0; i < cmd.aliases().size(); ++i) {
            if (i > 0) ss << ", ";
            ss << cmd.aliases()[i];
        }
        ss << "\n\n";
    }
    
    ss << "DESCRIPTION\n";
    ss << cmd.help() << "\n\n";
    
    return ss.str();
}
```

## Next Steps

1. **Phase 1: Foundation**
   - Implement argument parser
   - Create command base interface
   - Build output formatters
   - Setup configuration system

2. **Phase 2: Core Commands**
   - Implement database commands
   - Implement data operations
   - Implement search commands
   - Add basic error handling

3. **Phase 3: Advanced Features**
   - Implement hybrid search commands
   - Add ingestion commands
   - Build index management
   - Add collection management

4. **Phase 4: Interactive**
   - Build interactive shell
   - Add tab completion
   - Implement command history
   - Add progress indicators

5. **Phase 5: Polish**
   - Comprehensive testing
   - Documentation
   - Error message improvement
   - Performance optimization

6. **Phase 6: Extended Features**
   - Distributed commands
   - RAG commands
   - Monitoring commands
   - Advanced utilities
