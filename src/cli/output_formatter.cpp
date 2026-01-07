#include "vdb/cli/output_formatter.hpp"
#include <iomanip>
#include <algorithm>

namespace vdb::cli {

OutputFormatter::OutputFormatter(Format format) 
    : format_(format) {
}

std::string OutputFormatter::format_table(
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
) const {
    if (format_ == Format::JSON) {
        // Simple JSON array output
        std::ostringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < rows.size(); ++i) {
            ss << "  {";
            for (size_t j = 0; j < headers.size() && j < rows[i].size(); ++j) {
                if (j > 0) ss << ", ";
                ss << "\"" << headers[j] << "\": \"" << rows[i][j] << "\"";
            }
            ss << "}";
            if (i < rows.size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "]\n";
        return ss.str();
    } else if (format_ == Format::CSV) {
        std::ostringstream ss;
        // Headers
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) ss << ",";
            ss << headers[i];
        }
        ss << "\n";
        // Rows
        for (const auto& row : rows) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i > 0) ss << ",";
                ss << row[i];
            }
            ss << "\n";
        }
        return ss.str();
    } else {
        // Table format
        if (rows.empty()) {
            return "No results\n";
        }
        
        // Calculate column widths
        std::vector<size_t> widths(headers.size(), 0);
        for (size_t i = 0; i < headers.size(); ++i) {
            widths[i] = headers[i].size();
        }
        for (const auto& row : rows) {
            for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
                widths[i] = std::max(widths[i], row[i].size());
            }
        }
        
        std::ostringstream ss;
        
        // Top border
        ss << "┌";
        for (size_t i = 0; i < widths.size(); ++i) {
            if (i > 0) ss << "┬";
            ss << std::string(widths[i] + 2, '─');
        }
        ss << "┐\n";
        
        // Headers
        ss << "│";
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) ss << "│";
            ss << " " << std::left << std::setw(widths[i]) << headers[i] << " ";
        }
        ss << "│\n";
        
        // Separator
        ss << "├";
        for (size_t i = 0; i < widths.size(); ++i) {
            if (i > 0) ss << "┼";
            ss << std::string(widths[i] + 2, '─');
        }
        ss << "┤\n";
        
        // Rows
        for (const auto& row : rows) {
            ss << "│";
            for (size_t i = 0; i < widths.size(); ++i) {
                if (i > 0) ss << "│";
                std::string val = i < row.size() ? row[i] : "";
                ss << " " << std::left << std::setw(widths[i]) << val << " ";
            }
            ss << "│\n";
        }
        
        // Bottom border
        ss << "└";
        for (size_t i = 0; i < widths.size(); ++i) {
            if (i > 0) ss << "┴";
            ss << std::string(widths[i] + 2, '─');
        }
        ss << "┘\n";
        
        return ss.str();
    }
}

std::string OutputFormatter::format_keyvalue(
    const std::vector<std::pair<std::string, std::string>>& data
) const {
    if (format_ == Format::JSON) {
        std::ostringstream ss;
        ss << "{\n";
        for (size_t i = 0; i < data.size(); ++i) {
            if (i > 0) ss << ",\n";
            ss << "  \"" << data[i].first << "\": \"" << data[i].second << "\"";
        }
        ss << "\n}\n";
        return ss.str();
    } else if (format_ == Format::CSV) {
        std::ostringstream ss;
        ss << "key,value\n";
        for (const auto& [key, value] : data) {
            ss << key << "," << value << "\n";
        }
        return ss.str();
    } else {
        // Find max key length for alignment
        size_t max_len = 0;
        for (const auto& [key, _] : data) {
            max_len = std::max(max_len, key.size());
        }
        
        std::ostringstream ss;
        for (const auto& [key, value] : data) {
            ss << std::left << std::setw(max_len + 2) << (key + ":")
               << value << "\n";
        }
        return ss.str();
    }
}

std::string OutputFormatter::format_success(const std::string& message) const {
    if (format_ == Format::JSON) {
        return "{\"status\": \"success\", \"message\": \"" + message + "\"}\n";
    } else {
        return "✓ " + message + "\n";
    }
}

std::string OutputFormatter::format_error(const std::string& message) const {
    if (format_ == Format::JSON) {
        return "{\"status\": \"error\", \"message\": \"" + message + "\"}\n";
    } else {
        return "✗ Error: " + message + "\n";
    }
}

} // namespace vdb::cli
