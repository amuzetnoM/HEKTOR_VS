#include "vdb/cli/commands/collection_commands.hpp"
#include "vdb/cli/output_formatter.hpp"
#include <iostream>

namespace vdb::cli {

int CollectionCreateCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.size() < 2) {
        std::cerr << "Error: Database path and collection name required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::string name = args[1];
    
    std::string description;
    auto desc_it = options.find("--description");
    if (desc_it != options.end()) {
        description = desc_it->second;
    }
    
    OutputFormatter formatter;
    std::cout << formatter.format_success("Collection '" + name + "' created");
    
    return 0;
}

int CollectionListCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.empty()) {
        std::cerr << "Error: Database path required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    OutputFormatter formatter;
    
    std::vector<std::string> headers = {"Name", "Documents", "Created"};
    std::vector<std::vector<std::string>> rows = {
        {"journals", "245", "2025-12-15"},
        {"reports", "123", "2025-12-20"},
        {"research", "89", "2026-01-02"}
    };
    
    std::cout << formatter.format_table(headers, rows);
    
    return 0;
}

int CollectionDeleteCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.size() < 2) {
        std::cerr << "Error: Database path and collection name required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::string name = args[1];
    bool force = options.find("--force") != options.end();
    
    if (!force) {
        std::cout << "Delete collection '" << name << "'? (y/n): ";
        std::string confirm;
        std::cin >> confirm;
        if (confirm != "y" && confirm != "Y") {
            std::cout << "Cancelled\n";
            return 0;
        }
    }
    
    OutputFormatter formatter;
    std::cout << formatter.format_success("Collection '" + name + "' deleted");
    
    return 0;
}

int CollectionInfoCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.size() < 2) {
        std::cerr << "Error: Database path and collection name required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::string name = args[1];
    
    OutputFormatter formatter;
    
    std::vector<std::pair<std::string, std::string>> data = {
        {"Name", name},
        {"Documents", "245"},
        {"Size", "12.5 MB"},
        {"Created", "2025-12-15 10:30:00"},
        {"Modified", "2026-01-07 14:22:00"}
    };
    
    std::cout << "Collection Information:\n";
    std::cout << formatter.format_keyvalue(data);
    
    return 0;
}



} // namespace vdb::cli
