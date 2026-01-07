#pragma once

#include "vdb/cli/command_base.hpp"
#include "vdb/database.hpp"
#include <memory>

namespace vdb::cli {

/**
 * hektor init - Initialize a new database
 */
class DbInitCommand : public CommandBase {
public:
    std::string name() const override { return "init"; }
    std::string description() const override { 
        return "Initialize a new vector database"; 
    }
    
    std::string usage() const override {
        return "hektor init <path> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Initialize a new vector database

Options:
  --dimension DIM      Vector dimension (default: 512)
  --metric METRIC      Distance metric: cosine|euclidean|dot (default: cosine)
  --preset PRESET      Use preset config: gold-standard|default

Examples:
  hektor init ./mydb
  hektor init ./mydb --dimension 384
  hektor init ./mydb --preset gold-standard
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor info - Show database information
 */
class DbInfoCommand : public CommandBase {
public:
    std::string name() const override { return "db:info"; }
    std::string description() const override { 
        return "Show database information and statistics"; 
    }
    
    std::string usage() const override {
        return "hektor info <path>";
    }
    
    std::string help() const override {
        return R"(Show database information and statistics

Examples:
  hektor info ./mydb
  hektor db:info ./mydb
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
