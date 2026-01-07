#pragma once

#include "vdb/cli/command_base.hpp"

namespace vdb::cli {

/**
 * hektor collection create - Create collection
 */
class CollectionCreateCommand : public CommandBase {
public:
    std::string name() const override { return "collection:create"; }
    std::string description() const override { 
        return "Create a new collection"; 
    }
    
    std::string usage() const override {
        return "hektor collection create <db> <name> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Create a new collection

Options:
  --description TEXT   Collection description

Examples:
  hektor collection create ./mydb journals
  hektor col create ./mydb reports --description "Market reports"
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor collection list - List collections
 */
class CollectionListCommand : public CommandBase {
public:
    std::string name() const override { return "collection:list"; }
    std::string description() const override { 
        return "List all collections"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"collection:ls"}; 
    }
    
    std::string usage() const override {
        return "hektor collection list <db>";
    }
    
    std::string help() const override {
        return R"(List all collections

Examples:
  hektor collection list ./mydb
  hektor col ls ./mydb
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor collection delete - Delete collection
 */
class CollectionDeleteCommand : public CommandBase {
public:
    std::string name() const override { return "collection:delete"; }
    std::string description() const override { 
        return "Delete a collection"; 
    }
    
    std::string usage() const override {
        return "hektor collection delete <db> <name> [--force]";
    }
    
    std::string help() const override {
        return R"(Delete a collection

Options:
  --force              Skip confirmation

Examples:
  hektor collection delete ./mydb journals
  hektor col delete ./mydb reports --force
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor collection info - Show collection info
 */
class CollectionInfoCommand : public CommandBase {
public:
    std::string name() const override { return "collection:info"; }
    std::string description() const override { 
        return "Show collection information"; 
    }
    
    std::string usage() const override {
        return "hektor collection info <db> <name>";
    }
    
    std::string help() const override {
        return R"(Show collection information

Examples:
  hektor collection info ./mydb journals
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
