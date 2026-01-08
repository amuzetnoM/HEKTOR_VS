// ============================================================================
// Hektor CLI - Main Entry Point
// ============================================================================

#include "vdb/cli/cli.hpp"
#include <iostream>

int main(int argc, char** argv) {
    try {
        vdb::cli::CLI cli(argc, argv);
        return cli.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error\n";
        return 1;
    }
}
