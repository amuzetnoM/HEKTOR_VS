// ============================================================================
// VectorDB - Command Line Interface
// ============================================================================

#include "vdb/database.hpp"
#include "vdb/ingest.hpp"
#include <iostream>
#include <iomanip>
#include <fmt/core.h>
#include <fmt/color.h>

using namespace vdb;

// ============================================================================
// CLI Helpers
// ============================================================================

void print_banner() {
    fmt::print(fg(fmt::color::gold), R"(
╔═══════════════════════════════════════════════════════════════════════════╗
║                                                                           ║
║  ██╗   ██╗███████╗ ██████╗████████╗ ██████╗ ██████╗     ██████╗ ██████╗   ║
║  ██║   ██║██╔════╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗    ██╔══██╗██╔══██╗  ║
║  ██║   ██║█████╗  ██║        ██║   ██║   ██║██████╔╝    ██║  ██║██████╔╝  ║
║  ╚██╗ ██╔╝██╔══╝  ██║        ██║   ██║   ██║██╔══██╗    ██║  ██║██╔══██╗  ║
║   ╚████╔╝ ███████╗╚██████╗   ██║   ╚██████╔╝██║  ██║    ██████╔╝██████╔╝  ║
║    ╚═══╝  ╚══════╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝    ╚═════╝ ╚═════╝   ║
║                                                                           ║
║           High-Performance Vector Database for Gold Standard              ║
╚═══════════════════════════════════════════════════════════════════════════╝
)");
    fmt::print("\n");
}

void print_usage() {
    fmt::print("Usage: vdb_cli <command> [options]\n\n");
    fmt::print("Commands:\n");
    fmt::print("  init <path>              Initialize a new database\n");
    fmt::print("  ingest <db> <gs_output>  Ingest Gold Standard outputs\n");
    fmt::print("  search <db> <query>      Search the database\n");
    fmt::print("  stats <db>               Show database statistics\n");
    fmt::print("  export <db> <output>     Export training data\n");
    fmt::print("  help                     Show this help message\n");
    fmt::print("\nOptions:\n");
    fmt::print("  -k <num>                 Number of results (default: 10)\n");
    fmt::print("  --type <type>            Filter by document type\n");
    fmt::print("  --date <date>            Filter by date (YYYY-MM-DD)\n");
    fmt::print("  --asset <asset>          Filter by asset (GOLD, SILVER, etc.)\n");
}

void print_stats(const IndexStats& stats) {
    fmt::print(fg(fmt::color::cyan), "\n═══ Database Statistics ═══\n\n");
    fmt::print("  Total Vectors:    {:>10}\n", stats.total_vectors);
    fmt::print("  Dimension:        {:>10}\n", stats.dimension);
    fmt::print("  Index Type:       {:>10}\n", stats.index_type);
    fmt::print("  Memory Usage:     {:>10} MB\n", stats.memory_usage_bytes / 1024 / 1024);
    fmt::print("  Index Size:       {:>10} KB\n", stats.index_size_bytes / 1024);
    fmt::print("\n");
}

void print_results(const QueryResults& results) {
    fmt::print(fg(fmt::color::green), "\n═══ Search Results ({} found) ═══\n\n", results.size());
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        fmt::print(fg(fmt::color::yellow), "{}. ", i + 1);
        fmt::print("Score: {:.4f}", r.score);
        
        if (r.metadata) {
            fmt::print(" | Type: {}", document_type_name(r.metadata->type));
            if (!r.metadata->date.empty()) {
                fmt::print(" | Date: {}", r.metadata->date);
            }
            if (!r.metadata->asset.empty()) {
                fmt::print(" | Asset: {}", r.metadata->asset);
            }
            if (!r.metadata->bias.empty()) {
                fmt::print(" | Bias: {}", r.metadata->bias);
            }
        }
        fmt::print("\n");
        
        if (r.metadata && !r.metadata->source_file.empty()) {
            fmt::print(fg(fmt::color::dim_gray), "   Source: {}\n", r.metadata->source_file);
        }
    }
    fmt::print("\n");
}

void print_ingest_stats(const IngestStats& stats) {
    fmt::print(fg(fmt::color::green), "\n═══ Ingest Complete ═══\n\n");
    fmt::print("  Journals Added:   {:>6}\n", stats.journals_added);
    fmt::print("  Charts Added:     {:>6}\n", stats.charts_added);
    fmt::print("  Reports Added:    {:>6}\n", stats.reports_added);
    fmt::print("  Skipped:          {:>6}\n", stats.skipped);
    
    if (stats.errors > 0) {
        fmt::print(fg(fmt::color::red), "  Errors:           {:>6}\n", stats.errors);
        for (const auto& msg : stats.error_messages) {
            fmt::print(fg(fmt::color::red), "    - {}\n", msg);
        }
    }
    fmt::print("\n");
}

// ============================================================================
// Commands
// ============================================================================

int cmd_init(const std::string& path) {
    fmt::print("Initializing database at: {}\n", path);
    
    auto result = create_gold_standard_db(path);
    if (!result) {
        fmt::print(fg(fmt::color::red), "Error: {}\n", result.error().message);
        return 1;
    }
    
    fmt::print(fg(fmt::color::green), "Database initialized successfully!\n");
    print_stats(result->stats());
    return 0;
}

int cmd_ingest(const std::string& db_path, const std::string& gs_output) {
    fmt::print("Opening database: {}\n", db_path);
    
    auto db_result = open_database(db_path);
    if (!db_result) {
        fmt::print(fg(fmt::color::red), "Error opening database: {}\n", db_result.error().message);
        return 1;
    }
    
    auto& db = *db_result;
    GoldStandardIngest ingest(db);
    
    IngestConfig config;
    config.gold_standard_output = gs_output;
    config.include_journals = true;
    config.include_charts = true;
    config.include_reports = true;
    config.incremental = true;
    
    fmt::print("Ingesting from: {}\n", gs_output);
    
    auto result = ingest.ingest(config);
    if (!result) {
        fmt::print(fg(fmt::color::red), "Error: {}\n", result.error().message);
        return 1;
    }
    
    print_ingest_stats(*result);
    return 0;
}

int cmd_search(const std::string& db_path, const std::string& query, size_t k) {
    auto db_result = open_database(db_path);
    if (!db_result) {
        fmt::print(fg(fmt::color::red), "Error: {}\n", db_result.error().message);
        return 1;
    }
    
    auto& db = *db_result;
    
    QueryOptions opts;
    opts.k = k;
    opts.include_metadata = true;
    
    fmt::print("Searching for: \"{}\"\n", query);
    
    auto results = db.query_text(query, opts);
    if (!results) {
        fmt::print(fg(fmt::color::red), "Error: {}\n", results.error().message);
        return 1;
    }
    
    print_results(*results);
    return 0;
}

int cmd_stats(const std::string& db_path) {
    auto db_result = open_database(db_path);
    if (!db_result) {
        fmt::print(fg(fmt::color::red), "Error: {}\n", db_result.error().message);
        return 1;
    }
    
    auto& db = *db_result;
    print_stats(db.stats());
    
    // Count by type
    fmt::print("Documents by Type:\n");
    for (int t = 0; t <= static_cast<int>(DocumentType::Unknown); ++t) {
        auto type = static_cast<DocumentType>(t);
        size_t count = db.count_by_type(type);
        if (count > 0) {
            fmt::print("  {:<25} {:>6}\n", document_type_name(type), count);
        }
    }
    
    // Show dates
    auto dates = db.all_dates();
    if (!dates.empty()) {
        fmt::print("\nDate Range: {} to {} ({} days)\n", 
                   dates.front(), dates.back(), dates.size());
    }
    
    return 0;
}

int cmd_export(const std::string& db_path, const std::string& output_path) {
    auto db_result = open_database(db_path);
    if (!db_result) {
        fmt::print(fg(fmt::color::red), "Error: {}\n", db_result.error().message);
        return 1;
    }
    
    auto& db = *db_result;
    
    fmt::print("Exporting to: {}\n", output_path);
    
    auto result = db.export_training_data(output_path);
    if (!result) {
        fmt::print(fg(fmt::color::red), "Error: {}\n", result.error().message);
        return 1;
    }
    
    fmt::print(fg(fmt::color::green), "Export complete!\n");
    return 0;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_banner();
        print_usage();
        return 0;
    }
    
    std::string command = argv[1];
    
    if (command == "help" || command == "--help" || command == "-h") {
        print_banner();
        print_usage();
        return 0;
    }
    
    if (command == "init") {
        if (argc < 3) {
            fmt::print(fg(fmt::color::red), "Error: Missing database path\n");
            return 1;
        }
        return cmd_init(argv[2]);
    }
    
    if (command == "ingest") {
        if (argc < 4) {
            fmt::print(fg(fmt::color::red), "Error: Missing arguments\n");
            fmt::print("Usage: vdb_cli ingest <db_path> <gold_standard_output>\n");
            return 1;
        }
        return cmd_ingest(argv[2], argv[3]);
    }
    
    if (command == "search") {
        if (argc < 4) {
            fmt::print(fg(fmt::color::red), "Error: Missing arguments\n");
            fmt::print("Usage: vdb_cli search <db_path> <query>\n");
            return 1;
        }
        size_t k = 10;
        // Parse -k option
        for (int i = 4; i < argc - 1; ++i) {
            if (std::string(argv[i]) == "-k") {
                k = std::stoul(argv[i + 1]);
            }
        }
        return cmd_search(argv[2], argv[3], k);
    }
    
    if (command == "stats") {
        if (argc < 3) {
            fmt::print(fg(fmt::color::red), "Error: Missing database path\n");
            return 1;
        }
        return cmd_stats(argv[2]);
    }
    
    if (command == "export") {
        if (argc < 4) {
            fmt::print(fg(fmt::color::red), "Error: Missing arguments\n");
            fmt::print("Usage: vdb_cli export <db_path> <output_path>\n");
            return 1;
        }
        return cmd_export(argv[2], argv[3]);
    }
    
    fmt::print(fg(fmt::color::red), "Unknown command: {}\n", command);
    print_usage();
    return 1;
}
