#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace vdb::cli {

/**
 * Base class for all CLI commands
 */
class CommandBase {
public:
    virtual ~CommandBase() = default;
    
    // Command metadata
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual std::vector<std::string> aliases() const { return {}; }
    
    // Usage and help
    virtual std::string usage() const = 0;
    virtual std::string help() const = 0;
    
    // Execute the command
    // Returns 0 on success, non-zero on error
    virtual int execute(
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& options
    ) = 0;
};

} // namespace vdb::cli
