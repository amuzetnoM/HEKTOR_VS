#include "vdb/cli/commands/hybrid_commands.hpp"
#include "vdb/cli/output_formatter.hpp"
#include <iostream>
#include <filesystem>

namespace vdb::cli {

int HybridSearchCommand::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.size() < 2) {
        std::cerr << "Error: Database path and query required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::string query = args[1];
    
    // Get options
    int k = 10;
    auto k_it = options.find("-k");
    if (k_it != options.end()) {
        k = std::stoi(k_it->second);
    }
    
    std::string fusion = "rrf";
    auto fusion_it = options.find("--fusion");
    if (fusion_it != options.end()) {
        fusion = fusion_it->second;
    }
    
    double vector_weight = 0.7;
    auto vw_it = options.find("--vector-weight");
    if (vw_it != options.end()) {
        vector_weight = std::stod(vw_it->second);
    }
    
    // Check database exists
    std::filesystem::path db_dir(db_path);
    if (!std::filesystem::exists(db_dir / "config.json")) {
        std::cerr << "Error: Database not found at " << db_path << "\n";
        return 1;
    }
    
    OutputFormatter formatter;
    
    // Create sample hybrid search results
    std::vector<std::string> headers = {"ID", "Score", "Vector", "BM25", "Preview"};
    std::vector<std::vector<std::string>> rows;
    
    std::cout << "Hybrid Search (Fusion: " << fusion << ")\n";
    std::cout << "Query: " << query << "\n";
    std::cout << "Vector Weight: " << vector_weight << ", Lexical Weight: " << (1.0 - vector_weight) << "\n\n";
    
    // Sample results
    for (int i = 1; i <= std::min(k, 5); ++i) {
        double vector_score = 0.95 - i * 0.05;
        double bm25_score = 0.85 - i * 0.08;
        double hybrid_score = vector_score * vector_weight + bm25_score * (1.0 - vector_weight);
        
        rows.push_back({
            std::to_string(10000 + i),
            std::to_string(hybrid_score).substr(0, 5),
            std::to_string(vector_score).substr(0, 4),
            std::to_string(bm25_score).substr(0, 4),
            "Document matching '" + query + "'..."
        });
    }
    
    std::cout << formatter.format_table(headers, rows);
    std::cout << "\n(Full hybrid search requires database engine integration)\n";
    
    return 0;
}

int HybridBM25Command::execute(
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& options
) {
    if (args.size() < 2) {
        std::cerr << "Error: Database path and query required\n";
        std::cerr << usage() << "\n";
        return 1;
    }
    
    std::string db_path = args[0];
    std::string query = args[1];
    
    int k = 10;
    auto k_it = options.find("-k");
    if (k_it != options.end()) {
        k = std::stoi(k_it->second);
    }
    
    OutputFormatter formatter;
    
    std::cout << "BM25 Full-Text Search\n";
    std::cout << "Query: " << query << "\n\n";
    
    std::vector<std::string> headers = {"ID", "BM25 Score", "Preview"};
    std::vector<std::vector<std::string>> rows;
    
    for (int i = 1; i <= std::min(k, 5); ++i) {
        rows.push_back({
            std::to_string(20000 + i),
            std::to_string(0.90 - i * 0.08).substr(0, 4),
            "Text matching '" + query + "'..."
        });
    }
    
    std::cout << formatter.format_table(headers, rows);
    std::cout << "\n(Full BM25 search requires database engine integration)\n";
    
    return 0;
}



} // namespace vdb::cli
