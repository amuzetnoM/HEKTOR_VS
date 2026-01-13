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



/**
 * hektor update - Update document
 */
class DataUpdateCommand : public CommandBase {
public:
    std::string name() const override { return "update"; }
    std::string description() const override { 
        return "Update existing document"; 
    }
    
    std::string usage() const override {
        return "hektor update <db> <id> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Update existing document

Options:
  --text TEXT          New text content
  --metadata JSON      Update metadata
  --tags TAG1,TAG2     Update tags

Examples:
  hektor update ./mydb 12345 --text "Updated content"
  hektor update ./mydb 12345 --metadata '{"status":"reviewed"}'
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor batch - Batch insert
 */
class DataBatchCommand : public CommandBase {
public:
    std::string name() const override { return "batch"; }
    std::string description() const override { 
        return "Batch insert from file"; 
    }
    
    std::string usage() const override {
        return "hektor batch <db> <file> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Batch insert from file

Options:
  --format FORMAT      json|jsonl|csv (default: jsonl)
  --workers N          Parallel workers (default: 4)
  --chunk-size N       Batch size (default: 100)
  --skip-errors        Continue on errors

Examples:
  hektor batch ./mydb documents.jsonl
  hektor batch ./mydb data.csv --format csv --workers 8
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor list - List documents
 */
class DataListCommand : public CommandBase {
public:
    std::string name() const override { return "list"; }
    std::string description() const override { 
        return "List documents in database"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"ls"}; 
    }
    
    std::string usage() const override {
        return "hektor list <db> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(List documents in database

Options:
  --limit N            Maximum documents (default: 100)
  --offset N           Skip N documents
  --type TYPE          Filter by type

Examples:
  hektor list ./mydb
  hektor ls ./mydb --limit 50 --type journal
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
