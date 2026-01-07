#include "vdb/cli/commands/db_commands.hpp"
#include "vdb/cli/output_formatter.hpp"
#include <iostream>
#include <filesystem>

namespace vdb::cli {

int DbInitCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.empty()) {
        std::cerr << "Error: Database path required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    int dimension = 512;
    std::string metric = "cosine";
    
    // Parse options
    auto dim_it = options.find("--dimension");
    if (dim_it != options.end()) {
        dimension = std::stoi(dim_it->second);
    }
    
    auto metric_it = options.find("--metric");
    if (metric_it != options.end()) {
        metric = metric_it->second;
    }
    
    auto preset_it = options.find("--preset");
    if (preset_it != options.end() && preset_it->second == "gold-standard") {
        dimension = 512;
        metric = "cosine";
    }
    
    // Create database directory
    try {
        std::filesystem::create_directories(db_path);
        
        // Create basic database files
        std::filesystem::path db_dir(db_path);
        
        // Create config file
        std::ofstream config_file(db_dir / "config.json");
        config_file << "{\n";
        config_file << "  \"dimension\": " << dimension << ",\n";
        config_file << "  \"metric\": \"" << metric << "\",\n";
        config_file << "  \"version\": \"2.3.0\"\n";
        config_file << "}\n";
        config_file.close();
        
        // Create empty vectors file
        std::ofstream vectors_file(db_dir / "vectors.bin");
        vectors_file.close();
        
        // Create empty metadata file
        std::ofstream metadata_file(db_dir / "metadata.json");
        metadata_file << "[]\n";
        metadata_file.close();
        
        OutputFormatter formatter;
        std::cout << formatter.format_success("Database initialized at " + db_path);
        std::cout << "\nConfiguration:\n";
        std::cout << "  Dimension: " << dimension << "\n";
        std::cout << "  Metric:    " << metric << "\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error creating database: " << e.what() << "\n";
        return 1;
    }
}

int DbInfoCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.empty()) {
        std::cerr << "Error: Database path required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::filesystem::path db_dir(db_path);
    
    // Check if database exists
    if (!std::filesystem::exists(db_dir / "config.json")) {
        std::cerr << "Error: Database not found at " << db_path << "\n";
        std::cerr << "Run 'hektor init " << db_path << "' to create it\n";
        return 1;
    }
    
    // Read config
    std::ifstream config_file(db_dir / "config.json");
    std::string config_content((std::istreambuf_iterator<char>(config_file)),
                               std::istreambuf_iterator<char>());
    
    // Count vectors (simplified)
    size_t vector_count = 0;
    if (std::filesystem::exists(db_dir / "metadata.json")) {
        std::ifstream metadata_file(db_dir / "metadata.json");
        std::string metadata_content((std::istreambuf_iterator<char>(metadata_file)),
                                     std::istreambuf_iterator<char>());
        // Simple count based on '},' occurrences
        size_t pos = 0;
        while ((pos = metadata_content.find("},{", pos)) != std::string::npos) {
            ++vector_count;
            pos += 3;
        }
        if (metadata_content.find('{') != std::string::npos) {
            ++vector_count;
        }
    }
    
    // Display info
    OutputFormatter formatter;
    std::vector<std::pair<std::string, std::string>> data = {
        {"Database Path", db_path},
        {"Vector Count", std::to_string(vector_count)},
        {"Status", "Ready"}
    };
    
    std::cout << "Database Information:\n";
    std::cout << formatter.format_keyvalue(data);
    
    return 0;
}

} // namespace vdb::cli
