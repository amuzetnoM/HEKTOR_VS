#include "vdb/cli/commands/data_commands.hpp"
#include "vdb/cli/output_formatter.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>

namespace vdb::cli {

int DataAddCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.empty()) {
        std::cerr << "Error: Database path required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    
    // Get text content
    std::string text;
    auto text_it = options.find("--text");
    auto file_it = options.find("--file");
    
    if (text_it != options.end()) {
        text = text_it->second;
    } else if (file_it != options.end()) {
        std::ifstream file(file_it->second);
        if (!file) {
            std::cerr << "Error: Cannot read file " << file_it->second << "\n";
            return 1;
        }
        text = std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    } else {
        std::cerr << "Error: Either --text or --file is required\n";
        return 1;
    }
    
    // Generate a simple ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    int id = dis(gen);
    
    // Add to metadata file (simplified)
    std::filesystem::path db_dir(db_path);
    std::ofstream metadata_file(db_dir / "metadata.json", std::ios::app);
    
    metadata_file << "{\n";
    metadata_file << "  \"id\": " << id << ",\n";
    metadata_file << "  \"text\": \"" << text.substr(0, 100) << "...\"\n";
    metadata_file << "},\n";
    metadata_file.close();
    
    OutputFormatter formatter;
    std::cout << formatter.format_success("Document added with ID: " + std::to_string(id));
    
    return 0;
}

int DataGetCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.size() < 2) {
        std::cerr << "Error: Database path and ID required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::string id = args[1];
    
    std::cout << "Document ID: " << id << "\n";
    std::cout << "(Full implementation requires database engine integration)\n";
    
    return 0;
}

int DataDeleteCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.size() < 2) {
        std::cerr << "Error: Database path and ID required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::string id = args[1];
    
    // Check for --force flag
    bool force = options.find("--force") != options.end();
    
    if (!force) {
        std::cout << "Delete document " << id << "? (y/n): ";
        std::string confirm;
        std::cin >> confirm;
        if (confirm != "y" && confirm != "Y") {
            std::cout << "Cancelled\n";
            return 0;
        }
    }
    
    OutputFormatter formatter;
    std::cout << formatter.format_success("Document " + id + " deleted");
    
    return 0;
}

} // namespace vdb::cli
