#pragma once

#include "vdb/cli/command_base.hpp"

namespace vdb::cli {

/**
 * hektor search - Search the database
 */
class SearchCommand : public CommandBase {
public:
    std::string name() const override { return "search"; }
    std::string description() const override { 
        return "Search the database with semantic query"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"s"}; 
    }
    
    std::string usage() const override {
        return "hektor search <db> <query> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Search the database with semantic query

Options:
  -k NUM               Number of results (default: 10)
  --type TYPE          Filter by document type
  --date-from DATE     Filter by date from (YYYY-MM-DD)
  --date-to DATE       Filter by date to (YYYY-MM-DD)
  --asset ASSET        Filter by asset

Examples:
  hektor search ./mydb "gold prices" -k 20
  hektor s ./mydb "outlook" --type journal
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
