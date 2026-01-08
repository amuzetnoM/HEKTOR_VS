#pragma once

#include "vdb/cli/command_base.hpp"

namespace vdb::cli {

/**
 * hektor hybrid search - Hybrid search (vector + BM25)
 */
class HybridSearchCommand : public CommandBase {
public:
    std::string name() const override { return "hybrid:search"; }
    std::string description() const override { 
        return "Hybrid search combining vector and BM25"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"hs"}; 
    }
    
    std::string usage() const override {
        return "hektor hybrid search <db> <query> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Hybrid search combining vector and BM25

Options:
  -k NUM               Number of results (default: 10)
  --fusion METHOD      Fusion: rrf|weighted|combsum|combmnz|borda (default: rrf)
  --vector-weight W    Vector weight (0.0-1.0, default: 0.7)
  --lexical-weight W   Lexical weight (0.0-1.0, default: 0.3)
  --rrf-k K            RRF k parameter (default: 60)

Examples:
  hektor hybrid search ./mydb "gold outlook" -k 20
  hektor hs ./mydb "analysis" --fusion weighted --vector-weight 0.8
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor hybrid bm25 - BM25 only search
 */
class HybridBM25Command : public CommandBase {
public:
    std::string name() const override { return "hybrid:bm25"; }
    std::string description() const override { 
        return "BM25 full-text search only"; 
    }
    
    std::string usage() const override {
        return "hektor hybrid bm25 <db> <query> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(BM25 full-text search only

Options:
  -k NUM               Number of results (default: 10)

Examples:
  hektor hybrid bm25 ./mydb "gold prices"
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
