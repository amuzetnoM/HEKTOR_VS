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

/**
 * hektor db optimize - Optimize database
 */
class DbOptimizeCommand : public CommandBase {
public:
    std::string name() const override { return "db:optimize"; }
    std::string description() const override { 
        return "Optimize database for better performance"; 
    }
    
    std::string usage() const override {
        return "hektor db optimize <path>";
    }
    
    std::string help() const override {
        return R"(Optimize database for better performance

Examples:
  hektor db optimize ./mydb
  hektor optimize ./mydb
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor db backup - Backup database
 */
class DbBackupCommand : public CommandBase {
public:
    std::string name() const override { return "db:backup"; }
    std::string description() const override { 
        return "Backup database to archive"; 
    }
    
    std::string usage() const override {
        return "hektor db backup <path> <dest> [OPTIONS]";
    }
    
    std::string help() const override {
        return R"(Backup database to archive

Options:
  --compress           Compress backup (default: true)

Examples:
  hektor db backup ./mydb ./backup.tar.gz
  hektor backup ./mydb ./backup.tar.gz
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor db restore - Restore database
 */
class DbRestoreCommand : public CommandBase {
public:
    std::string name() const override { return "db:restore"; }
    std::string description() const override { 
        return "Restore database from backup"; 
    }
    
    std::string usage() const override {
        return "hektor db restore <src> <path>";
    }
    
    std::string help() const override {
        return R"(Restore database from backup

Examples:
  hektor db restore ./backup.tar.gz ./mydb
  hektor restore ./backup.tar.gz ./mydb
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor db health - Health check
 */
class DbHealthCommand : public CommandBase {
public:
    std::string name() const override { return "db:health"; }
    std::string description() const override { 
        return "Check database health"; 
    }
    
    std::string usage() const override {
        return "hektor db health <path>";
    }
    
    std::string help() const override {
        return R"(Check database health

Examples:
  hektor db health ./mydb
  hektor health ./mydb
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

/**
 * hektor db list - List databases
 */
class DbListCommand : public CommandBase {
public:
    std::string name() const override { return "db:list"; }
    std::string description() const override { 
        return "List all databases"; 
    }
    std::vector<std::string> aliases() const override { 
        return {"db:ls"}; 
    }
    
    std::string usage() const override {
        return "hektor db list [--path DIR]";
    }
    
    std::string help() const override {
        return R"(List all databases

Options:
  --path DIR           Directory to scan (default: current)

Examples:
  hektor db list
  hektor db ls --path /data
)";
    }
    
    int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) override;
};

} // namespace vdb::cli
