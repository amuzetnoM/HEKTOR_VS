#include "vdb/cli/cli.hpp"
#include "vdb/cli/command_base.hpp"
#include "vdb/cli/output_formatter.hpp"
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
    std::cout << R"(
Hektor - High-Performance Vector Database CLI
Version 2.3.0 - Phase 2 Extended

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
    optimize <path>       Optimize database
    backup <path> <dest>  Backup database
    restore <src> <path>  Restore from backup
    health <path>         Health check
    db:list               List all databases
  
  Data Operations:
    add <db>              Add a document
    get <db> <id>         Get document by ID
    update <db> <id>      Update a document
    delete <db> <id>      Delete a document (alias: rm)
    batch <db> <file>     Batch insert from file
    list <db>             List documents (alias: ls)
  
  Search:
    search <db> <query>   Semantic search (alias: s)
    
  Hybrid Search:
    hybrid:search <db>    Hybrid vector+BM25 search (alias: hs)
    hybrid:bm25 <db>      BM25 full-text search only
  
  Ingestion:
    ingest <db> <source>  Import external data
    ingest:scan <source>  Scan source without importing
  
  Index Management:
    index:build <db>      Build or rebuild index
    index:optimize <db>   Optimize index
    index:stats <db>      Show index statistics
    index:benchmark <db>  Benchmark index performance
  
  Collections:
    collection:create     Create collection
    collection:list       List collections (alias: collection:ls)
    collection:delete     Delete collection
    collection:info       Show collection info
  
  Export:
    export:data <db>      Export database data
    export:pairs <db>     Export training pairs
    export:triplets <db>  Export training triplets
  
  General:
    help                  Show this help message
    version               Show version information

Examples:
  # Initialize and add documents
  hektor init ./mydb --preset gold-standard
  hektor add ./mydb --text "Gold prices rising"
  hektor batch ./mydb documents.jsonl

  # Search
  hektor search ./mydb "gold outlook" -k 20
  hektor hs ./mydb "analysis" --fusion rrf

  # Ingestion
  hektor ingest ./mydb ./docs --format pdf --recursive
  hektor ingest ./mydb data.csv --chunk-strategy sentence

  # Index management
  hektor index:build ./mydb --type hnsw --hnsw-m 32
  hektor index:benchmark ./mydb --queries 1000

  # Collections
  hektor collection:create ./mydb journals
  hektor collection:list ./mydb

  # Export for ML training
  hektor export:triplets ./mydb training.jsonl --negative-samples 10

  # Database maintenance
  hektor optimize ./mydb
  hektor backup ./mydb ./backup.tar.gz
  hektor health ./mydb

For detailed command help: hektor <command> --help
For more information: https://github.com/amuzetnoM/hektor
)";
}

void CLI::show_version() {
    std::cout << "Hektor CLI version 2.3.0 - Phase 2 Extended\n";
    std::cout << "Vector Database Engine\n";
    std::cout << "\nPhase 2 Features:\n";
    std::cout << "  • 35+ commands across 8 categories\n";
    std::cout << "  • Hybrid search (vector + BM25)\n";
    std::cout << "  • Data ingestion with 10+ adapters\n";
    std::cout << "  • Index management and optimization\n";
    std::cout << "  • Collection management\n";
    std::cout << "  • Export for ML training\n";
    std::cout << "  • Database backup/restore\n";
    std::cout << "  • Advanced data operations\n";
}

} // namespace vdb::cli
