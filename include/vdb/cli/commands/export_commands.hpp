#pragma once

#include "vdb/cli/command_base.hpp"

namespace vdb::cli {

/**
 * hektor export data - Export data
 */
class ExportDataCommand : public CommandBase {
public:
    std::string name() const override { return "export:data"; }
    std::string description() const override { 
        return "Export database data"; 
    }
    
    std::string usage() const override {
        return "hektor export data <db> <output> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Export database data

Options:
  --format FORMAT      json|jsonl|csv|parquet (default: jsonl)
  --include-vectors    Include vector embeddings

Examples:
  hektor export data ./mydb output.jsonl
  hektor export data ./mydb data.csv --format csv
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor export pairs - Export training pairs
 */
class ExportPairsCommand : public CommandBase {
public:
    std::string name() const override { return "export:pairs"; }
    std::string description() const override { 
        return "Export training pairs for model fine-tuning"; 
    }
    
    std::string usage() const override {
        return "hektor export pairs <db> <output> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Export training pairs for model fine-tuning

Options:
  --min-score SCORE    Minimum similarity score (default: 0.7)
  --limit N            Maximum pairs to export

Examples:
  hektor export pairs ./mydb training.jsonl
  hektor export pairs ./mydb pairs.json --min-score 0.8
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor export triplets - Export training triplets
 */
class ExportTripletsCommand : public CommandBase {
public:
    std::string name() const override { return "export:triplets"; }
    std::string description() const override { 
        return "Export training triplets (anchor, positive, negative)"; 
    }
    
    std::string usage() const override {
        return "hektor export triplets <db> <output> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Export training triplets (anchor, positive, negative)

Options:
  --negative-samples N  Negative samples per anchor (default: 5)
  --strategy STRATEGY   hard|random|semi-hard (default: hard)

Examples:
  hektor export triplets ./mydb training.jsonl
  hektor export triplets ./mydb triplets.json --negative-samples 10
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
