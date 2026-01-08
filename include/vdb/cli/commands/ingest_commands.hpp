#pragma once

#include "vdb/cli/command_base.hpp"

namespace vdb::cli {

/**
 * hektor ingest - Import external data
 */
class IngestCommand : public CommandBase {
public:
    std::string name() const override { return "ingest"; }
    std::string description() const override { 
        return "Import data from various sources"; 
    }
    
    std::string usage() const override {
        return "hektor ingest <db> <source> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Import data from various sources

Options:
  --format FORMAT      csv|json|pdf|excel|xml|parquet|sqlite|postgres|http
  --chunk-strategy S   fixed|sentence|paragraph|semantic|recursive
  --chunk-size N       Chunk size (default: 512)
  --overlap N          Chunk overlap (default: 50)
  --recursive          Recursively process directories
  --pattern PATTERN    File pattern (e.g., "*.pdf")
  --workers N          Parallel workers (default: 4)

Adapters:
  csv, json, jsonl     Text formats
  pdf, docx            Documents
  xlsx, xls            Spreadsheets
  xml, html            Markup
  parquet              Column format
  sqlite               SQLite database
  postgres://          PostgreSQL
  http://              HTTP API

Examples:
  hektor ingest ./mydb data.csv
  hektor ingest ./mydb ./docs --format pdf --recursive
  hektor ingest ./mydb data.xlsx --chunk-strategy sentence
  hektor ingest ./mydb http://api.example.com/data --format json
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor ingest scan - Scan source without importing
 */
class IngestScanCommand : public CommandBase {
public:
    std::string name() const override { return "ingest:scan"; }
    std::string description() const override { 
        return "Scan source to preview what would be ingested"; 
    }
    
    std::string usage() const override {
        return "hektor ingest scan <source> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Scan source to preview what would be ingested

Options:
  --format FORMAT      Source format
  --recursive          Scan directories recursively

Examples:
  hektor ingest scan ./data
  hektor ingest scan data.csv --format csv
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
