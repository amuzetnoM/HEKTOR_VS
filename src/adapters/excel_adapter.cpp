// ============================================================================
// VectorDB - Excel Data Adapter Implementation
// Note: Uses libxlsxwriter for writing. Reading requires additional library.
// ============================================================================

#include "vdb/adapters/excel_adapter.hpp"
#include <fstream>
#include <sstream>

namespace vdb::adapters {

ExcelAdapter::ExcelAdapter(const ExcelConfig& config) : config_(config) {}

bool ExcelAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".xlsx" || ext == ".xls";
}

bool ExcelAdapter::can_handle(std::string_view content) const {
    // Excel files start with PK (ZIP signature) for .xlsx
    // or specific header for .xls
    if (content.size() < 4) return false;
    
    // Check for ZIP signature (xlsx is a ZIP archive)
    if (content[0] == 'PK' && content[1] == '\x03' && content[2] == '\x04') {
        return true;
    }
    
    // Check for old Excel format (xls)
    if (content.size() >= 8) {
        // BOF record for XLS files
        return (static_cast<unsigned char>(content[0]) == 0xD0 && 
                static_cast<unsigned char>(content[1]) == 0xCF);
    }
    
    return false;
}

Result<NormalizedData> ExcelAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
#ifdef HAVE_XLSXWRITER
    // Note: libxlsxwriter is write-only. For reading, we need another library.
    // This is a placeholder that notes the requirement.
    return tl::unexpected(Error{
        ErrorCode::NotImplemented,
        "Excel reading requires additional library (xlnt or libxlsx). "
        "LibxlsxWriter supports writing only. Install xlnt for full Excel support."
    });
#else
    return tl::unexpected(Error{
        ErrorCode::NotImplemented,
        "Excel support requires libxlsxwriter (for writing) and xlnt (for reading). "
        "Compile with -DHAVE_XLSXWRITER and install xlnt library."
    });
#endif
}

Result<NormalizedData> ExcelAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    // Write to temp file and parse
    auto temp_path = fs::temp_directory_path() / "vdb_temp_excel.xlsx";
    {
        std::ofstream out(temp_path, std::ios::binary);
        out.write(content.data(), content.size());
    }
    
    auto result = parse(temp_path, config);
    fs::remove(temp_path);
    return result;
}

Result<void> ExcelAdapter::sanitize(NormalizedData& data) {
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
    }
    data.sanitized = true;
    return {};
}

Result<void> ExcelAdapter::write(
    const fs::path& path,
    const NormalizedData& data
) {
#ifdef HAVE_XLSXWRITER
    // Create workbook
    lxw_workbook* workbook = workbook_new(path.string().c_str());
    if (!workbook) {
        return tl::unexpected(Error{
            ErrorCode::IoError,
            "Failed to create Excel workbook: " + path.string()
        });
    }
    
    // Create worksheet
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Data");
    if (!worksheet) {
        workbook_close(workbook);
        return tl::unexpected(Error{
            ErrorCode::IoError,
            "Failed to create worksheet"
        });
    }
    
    // Write headers
    worksheet_write_string(worksheet, 0, 0, "Chunk Index", nullptr);
    worksheet_write_string(worksheet, 0, 1, "Content", nullptr);
    worksheet_write_string(worksheet, 0, 2, "Metadata", nullptr);
    
    // Write data chunks
    int row = 1;
    for (const auto& chunk : data.chunks) {
        worksheet_write_number(worksheet, row, 0, chunk.chunk_index, nullptr);
        worksheet_write_string(worksheet, row, 1, chunk.content.c_str(), nullptr);
        
        // Serialize metadata as JSON-like string
        std::string metadata_str;
        for (const auto& [key, value] : chunk.metadata) {
            metadata_str += key + "=" + value + "; ";
        }
        worksheet_write_string(worksheet, row, 2, metadata_str.c_str(), nullptr);
        
        ++row;
    }
    
    // Close workbook (writes file)
    lxw_error error = workbook_close(workbook);
    if (error != LXW_NO_ERROR) {
        return tl::unexpected(Error{
            ErrorCode::IoError,
            "Failed to write Excel file: " + std::string(lxw_strerror(error))
        });
    }
    
    return {};
#else
    return tl::unexpected(Error{
        ErrorCode::NotImplemented,
        "Excel write support requires libxlsxwriter library"
    });
#endif
}

} // namespace vdb::adapters
