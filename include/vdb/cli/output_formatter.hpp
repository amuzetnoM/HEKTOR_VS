#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace vdb::cli {

/**
 * Output formatter for different formats
 */
class OutputFormatter {
public:
    enum class Format {
        Table,
        JSON,
        CSV
    };
    
    explicit OutputFormatter(Format format = Format::Table);
    
    // Format a table
    std::string format_table(
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    ) const;
    
    // Format a simple key-value output
    std::string format_keyvalue(
        const std::vector<std::pair<std::string, std::string>>& data
    ) const;
    
    // Format success message
    std::string format_success(const std::string& message) const;
    
    // Format error message
    std::string format_error(const std::string& message) const;
    
    // Set format
    void set_format(Format format) { format_ = format; }
    
private:
    Format format_;
};

} // namespace vdb::cli
