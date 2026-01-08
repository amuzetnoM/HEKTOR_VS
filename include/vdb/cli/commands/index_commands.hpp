#pragma once

#include "vdb/cli/command_base.hpp"

namespace vdb::cli {

/**
 * hektor index build - Build index
 */
class IndexBuildCommand : public CommandBase {
public:
    std::string name() const override { return "index:build"; }
    std::string description() const override { 
        return "Build or rebuild index"; 
    }
    
    std::string usage() const override {
        return "hektor index build <db> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Build or rebuild index

Options:
  --type TYPE          Index type: hnsw|flat (default: hnsw)
  --hnsw-m M           HNSW M parameter (default: 16)
  --hnsw-ef EF         HNSW ef_construction (default: 200)
  --force              Rebuild if exists

Examples:
  hektor index build ./mydb
  hektor index build ./mydb --type hnsw --hnsw-m 32
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor index optimize - Optimize index
 */
class IndexOptimizeCommand : public CommandBase {
public:
    std::string name() const override { return "index:optimize"; }
    std::string description() const override { 
        return "Optimize index for better performance"; 
    }
    
    std::string usage() const override {
        return "hektor index optimize <db>";
    }
    
    std::string help() const override {
        return R"(Optimize index for better performance

Examples:
  hektor index optimize ./mydb
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor index stats - Show index statistics
 */
class IndexStatsCommand : public CommandBase {
public:
    std::string name() const override { return "index:stats"; }
    std::string description() const override { 
        return "Show index statistics"; 
    }
    
    std::string usage() const override {
        return "hektor index stats <db>";
    }
    
    std::string help() const override {
        return R"(Show index statistics

Examples:
  hektor index stats ./mydb
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor index benchmark - Benchmark index performance
 */
class IndexBenchmarkCommand : public CommandBase {
public:
    std::string name() const override { return "index:benchmark"; }
    std::string description() const override { 
        return "Benchmark index search performance"; 
    }
    
    std::string usage() const override {
        return "hektor index benchmark <db> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Benchmark index search performance

Options:
  --queries N          Number of test queries (default: 100)
  --k K                Top-k results (default: 10)

Examples:
  hektor index benchmark ./mydb --queries 1000
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
