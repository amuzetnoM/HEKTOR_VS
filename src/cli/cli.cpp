#include "vdb/cli/cli.hpp"
#include "vdb/cli/command_base.hpp"
#include "vdb/cli/output_formatter.hpp"
#include "vdb/cli/interactive_shell.hpp"
#include "vdb/cli/colors.hpp"
#include "vdb/cli/commands/db_commands.hpp"
#include "vdb/cli/commands/data_commands.hpp"
#include "vdb/cli/commands/search_commands.hpp"
#include "vdb/cli/commands/hybrid_commands.hpp"
#include "vdb/cli/commands/ingest_commands.hpp"
#include "vdb/cli/commands/index_commands.hpp"
#include "vdb/cli/commands/collection_commands.hpp"
#include "vdb/cli/commands/export_commands.hpp"
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
    // Initialize colors
    Colors::enable(Colors::supports_colors());
    
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
    
    // Handle interactive shell
    if (command_ == "shell" || command_ == "interactive" || command_ == "repl") {
        return run_interactive_shell();
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
    aliases_["init"] = db_init->name();
    
    auto db_info = std::make_shared<DbInfoCommand>();
    commands_[db_info->name()] = db_info;
    aliases_["info"] = db_info->name();
    
    auto db_optimize = std::make_shared<DbOptimizeCommand>();
    commands_[db_optimize->name()] = db_optimize;
    aliases_["optimize"] = db_optimize->name();
    
    auto db_backup = std::make_shared<DbBackupCommand>();
    commands_[db_backup->name()] = db_backup;
    aliases_["backup"] = db_backup->name();
    
    auto db_restore = std::make_shared<DbRestoreCommand>();
    commands_[db_restore->name()] = db_restore;
    aliases_["restore"] = db_restore->name();
    
    auto db_health = std::make_shared<DbHealthCommand>();
    commands_[db_health->name()] = db_health;
    aliases_["health"] = db_health->name();
    
    auto db_list = std::make_shared<DbListCommand>();
    commands_[db_list->name()] = db_list;
    for (const auto& alias : db_list->aliases()) {
        aliases_[alias] = db_list->name();
    }
    
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
    
    auto data_update = std::make_shared<DataUpdateCommand>();
    commands_[data_update->name()] = data_update;
    aliases_["update"] = data_update->name();
    
    auto data_batch = std::make_shared<DataBatchCommand>();
    commands_[data_batch->name()] = data_batch;
    aliases_["batch"] = data_batch->name();
    
    auto data_list = std::make_shared<DataListCommand>();
    commands_[data_list->name()] = data_list;
    for (const auto& alias : data_list->aliases()) {
        aliases_[alias] = data_list->name();
    }
    
    // Search command
    auto search = std::make_shared<SearchCommand>();
    commands_[search->name()] = search;
    for (const auto& alias : search->aliases()) {
        aliases_[alias] = search->name();
    }
    
    // Hybrid search commands
    auto hybrid_search = std::make_shared<HybridSearchCommand>();
    commands_[hybrid_search->name()] = hybrid_search;
    for (const auto& alias : hybrid_search->aliases()) {
        aliases_[alias] = hybrid_search->name();
    }
    
    auto hybrid_bm25 = std::make_shared<HybridBM25Command>();
    commands_[hybrid_bm25->name()] = hybrid_bm25;
    
    // Ingestion commands
    auto ingest = std::make_shared<IngestCommand>();
    commands_[ingest->name()] = ingest;
    
    auto ingest_scan = std::make_shared<IngestScanCommand>();
    commands_[ingest_scan->name()] = ingest_scan;
    
    // Index commands
    auto index_build = std::make_shared<IndexBuildCommand>();
    commands_[index_build->name()] = index_build;
    
    auto index_optimize = std::make_shared<IndexOptimizeCommand>();
    commands_[index_optimize->name()] = index_optimize;
    
    auto index_stats = std::make_shared<IndexStatsCommand>();
    commands_[index_stats->name()] = index_stats;
    
    auto index_benchmark = std::make_shared<IndexBenchmarkCommand>();
    commands_[index_benchmark->name()] = index_benchmark;
    
    // Collection commands
    auto col_create = std::make_shared<CollectionCreateCommand>();
    commands_[col_create->name()] = col_create;
    
    auto col_list = std::make_shared<CollectionListCommand>();
    commands_[col_list->name()] = col_list;
    for (const auto& alias : col_list->aliases()) {
        aliases_[alias] = col_list->name();
    }
    
    auto col_delete = std::make_shared<CollectionDeleteCommand>();
    commands_[col_delete->name()] = col_delete;
    
    auto col_info = std::make_shared<CollectionInfoCommand>();
    commands_[col_info->name()] = col_info;
    
    // Export commands
    auto export_data = std::make_shared<ExportDataCommand>();
    commands_[export_data->name()] = export_data;
    
    auto export_pairs = std::make_shared<ExportPairsCommand>();
    commands_[export_pairs->name()] = export_pairs;
    
    auto export_triplets = std::make_shared<ExportTripletsCommand>();
    commands_[export_triplets->name()] = export_triplets;
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
    std::cout << Colors::bold(Colors::blue("\nHektor - High-Performance Vector Database CLI\n"));
    std::cout << Colors::gray("Version 2.3.0 - Phase 3: Interactive Mode + Advanced Features\n\n");
    
    std::cout << Colors::bold("Usage: ") << "hektor [OPTIONS] <COMMAND> [ARGS]\n\n";
    
    std::cout << Colors::bold("Global Options:\n");
    std::cout << "  -v, --verbose         Verbose output\n";
    std::cout << "  -q, --quiet           Quiet mode\n";
    std::cout << "  -d, --debug           Debug mode\n";
    std::cout << "  -f, --format FORMAT   Output format (table|json|csv)\n";
    std::cout << "  -o, --output FILE     Write output to file\n";
    std::cout << "  -h, --help            Show help\n";
    std::cout << "      --version         Show version\n\n";
    
    std::cout << Colors::cyan("Commands:\n");
    std::cout << Colors::bold("  Database Management:\n");
    std::cout << "    init <path>           Initialize a new database\n";
    std::cout << "    info <path>           Show database information\n";
    std::cout << "    optimize <path>       Optimize database\n";
    std::cout << "    backup <path> <dest>  Backup database\n";
    std::cout << "    restore <src> <path>  Restore from backup\n";
    std::cout << "    health <path>         Health check\n";
    std::cout << "    db:list               List all databases\n\n";
    
    std::cout << Colors::bold("  Data Operations:\n");
    std::cout << "    add <db>              Add a document\n";
    std::cout << "    get <db> <id>         Get document by ID\n";
    std::cout << "    update <db> <id>      Update a document\n";
    std::cout << "    delete <db> <id>      Delete a document (alias: rm)\n";
    std::cout << "    batch <db> <file>     Batch insert from file\n";
    std::cout << "    list <db>             List documents (alias: ls)\n\n";
    
    std::cout << Colors::bold("  Search:\n");
    std::cout << "    search <db> <query>   Semantic search (alias: s)\n\n";
    
    std::cout << Colors::bold("  Hybrid Search:\n");
    std::cout << "    hybrid:search <db>    Hybrid vector+BM25 search (alias: hs)\n";
    std::cout << "    hybrid:bm25 <db>      BM25 full-text search only\n\n";
    
    std::cout << Colors::bold("  Ingestion:\n");
    std::cout << "    ingest <db> <source>  Import external data\n";
    std::cout << "    ingest:scan <source>  Scan source without importing\n\n";
    
    std::cout << Colors::bold("  Index Management:\n");
    std::cout << "    index:build <db>      Build or rebuild index\n";
    std::cout << "    index:optimize <db>   Optimize index\n";
    std::cout << "    index:stats <db>      Show index statistics\n";
    std::cout << "    index:benchmark <db>  Benchmark index performance\n\n";
    
    std::cout << Colors::bold("  Collections:\n");
    std::cout << "    collection:create     Create collection\n";
    std::cout << "    collection:list       List collections (alias: col:ls)\n";
    std::cout << "    collection:delete     Delete collection\n";
    std::cout << "    collection:info       Show collection info\n\n";
    
    std::cout << Colors::bold("  Export:\n");
    std::cout << "    export:data <db>      Export database data\n";
    std::cout << "    export:pairs <db>     Export training pairs\n";
    std::cout << "    export:triplets <db>  Export training triplets\n\n";
    
    std::cout << Colors::bold("  Interactive Mode:\n");
    std::cout << Colors::highlight("    shell [db]            ") << "Start interactive REPL shell\n";
    std::cout << Colors::highlight("    repl [db]             ") << "Alias for shell\n";
    std::cout << Colors::highlight("    interactive [db]      ") << "Alias for shell\n\n";
    
    std::cout << Colors::bold("  General:\n");
    std::cout << "    help                  Show this help message\n";
    std::cout << "    version               Show version information\n\n";
    
    std::cout << Colors::bold("Examples:\n");
    std::cout << Colors::gray("  # Interactive shell\n");
    std::cout << "  hektor shell ./mydb\n\n";
    
    std::cout << Colors::gray("  # Initialize and add documents\n");
    std::cout << "  hektor init ./mydb --preset gold-standard\n";
    std::cout << "  hektor add ./mydb --text \"Gold prices rising\"\n";
    std::cout << "  hektor batch ./mydb documents.jsonl\n\n";
    
    std::cout << Colors::gray("  # Search\n");
    std::cout << "  hektor search ./mydb \"gold outlook\" -k 20\n";
    std::cout << "  hektor hs ./mydb \"analysis\" --fusion rrf\n\n";
    
    std::cout << Colors::gray("  # Ingestion\n");
    std::cout << "  hektor ingest ./mydb ./docs --format pdf --recursive\n";
    std::cout << "  hektor ingest ./mydb data.csv --chunk-strategy sentence\n\n";
    
    std::cout << Colors::gray("  # Index management\n");
    std::cout << "  hektor index:build ./mydb --type hnsw --hnsw-m 32\n";
    std::cout << "  hektor index:benchmark ./mydb --queries 1000\n\n";
    
    std::cout << Colors::gray("For detailed command help: ") << "hektor <command> --help\n";
    std::cout << Colors::gray("For more information: ") << Colors::cyan("https://github.com/amuzetnoM/hektor\n\n");
}

void CLI::show_version() {
    std::cout << Colors::bold(Colors::blue("Hektor Vector Database\n"));
    std::cout << Colors::cyan("Version: ") << "2.3.0\n";
    std::cout << Colors::cyan("Build: ") << "Phase 3 - Interactive Mode + Advanced Features\n";
    std::cout << Colors::cyan("Features: ") << "43+ Commands, Interactive REPL, Progress Indicators, Color Output\n";
    std::cout << Colors::gray("Copyright © 2025 Hektor Project\n");
}

int CLI::run_interactive_shell() {
    std::string db_path;
    
    // Get database path if provided
    if (!args_.empty()) {
        db_path = args_[0];
    }
    
    InteractiveShell shell(this, db_path);
    return shell.run();
}

} // namespace vdb::cli
