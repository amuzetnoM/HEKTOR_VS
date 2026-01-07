#pragma once

#include "vdb/cli/command_base.hpp"

namespace vdb::cli {

/**
 * hektor add - Add a document
 */
class DataAddCommand : public CommandBase {
public:
    std::string name() const override { return "add"; }
    std::string description() const override { 
        return "Add a document to the database"; 
    }
    
    std::string usage() const override {
        return "hektor add <db> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Add a document to the database

Options:
  --text TEXT          Document text content
  --file FILE          Read content from file
  --type TYPE          Document type (journal, report, etc.)
  --date DATE          Date (YYYY-MM-DD)
  --asset ASSET        Asset name (GOLD, SILVER, etc.)

Examples:
  hektor add ./mydb --text "Gold prices rising"
  hektor add ./mydb --file document.txt --type journal
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor get - Get document by ID
 */
class DataGetCommand : public CommandBase {
public:
    std::string name() const override { return "get"; }
    std::string description() const override { 
        return "Get document by ID"; 
    }
    
    std::string usage() const override {
        return "hektor get <db> <id>";
    }
    
    std::string help() const override {
        return R"(Get document by ID

Examples:
  hektor get ./mydb 12345
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor delete - Delete a document
 */
class DataDeleteCommand : public CommandBase {
public:
    std::string name() const override { return "delete"; }
    std::string description() const override { 
        return "Delete a document from the database"; 
    }
    
    std::string usage() const override {
        return "hektor delete <db> <id>";
    }
    
    std::string help() const override {
        return R"(Delete a document from the database

Options:
  --force              Skip confirmation

Examples:
  hektor delete ./mydb 12345
  hektor rm ./mydb 12345
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
