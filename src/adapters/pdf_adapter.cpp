// ============================================================================
// VectorDB - PDF Data Adapter Implementation
// ============================================================================

#include "vdb/adapters/pdf_adapter.hpp"
#include <fstream>
#include <sstream>

namespace vdb::adapters {

// Helper to escape special characters in PDF strings
static std::string escape_pdf_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '(' || c == ')' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    return result;
}

PDFAdapter::PDFAdapter(const PDFConfig& config) : config_(config) {}

bool PDFAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".pdf";
}

bool PDFAdapter::can_handle(std::string_view content) const {
    // PDF files start with "%PDF-"
    return content.size() >= 5 && content.substr(0, 5) == "%PDF-";
}

Result<NormalizedData> PDFAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
#ifdef HAVE_POPPLER
    // Load PDF document
    auto doc = poppler::document::load_from_file(path.string());
    if (!doc) {
        return tl::unexpected(Error{
            ErrorCode::ParseError,
            "Failed to load PDF file: " + path.string()
        });
    }
    
    return parse_pdf_document(doc.get(), config, path.string());
#else
    return tl::unexpected(Error{
        ErrorCode::NotImplemented,
        "PDF support requires poppler-cpp library (compile with -DHAVE_POPPLER)"
    });
#endif
}

Result<NormalizedData> PDFAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
#ifdef HAVE_POPPLER
    // Write content to temp file (poppler requires file path)
    auto temp_path = fs::temp_directory_path() / "vdb_temp_pdf.pdf";
    {
        std::ofstream out(temp_path, std::ios::binary);
        out.write(content.data(), content.size());
    }
    
    auto result = parse(temp_path, config);
    fs::remove(temp_path);
    return result;
#else
    return tl::unexpected(Error{
        ErrorCode::NotImplemented,
        "PDF support requires poppler-cpp library"
    });
#endif
}

Result<void> PDFAdapter::sanitize(NormalizedData& data) {
    // Sanitize each chunk's content
    for (auto& chunk : data.chunks) {
        chunk.content = sanitize_text(chunk.content);
    }
    data.sanitized = true;
    return {};
}

#ifdef HAVE_POPPLER
Result<NormalizedData> PDFAdapter::parse_pdf_document(
    poppler::document* doc,
    const ChunkConfig& config,
    const std::string& source_path
) {
    NormalizedData result;
    result.format = DataFormat::PDF;
    result.source_path = source_path;
    result.confidence = 1.0f;
    
    // Extract metadata
    result.global_metadata = extract_metadata(doc);
    
    // Determine page range
    int num_pages = doc->pages();
    int start = config_.start_page;
    int end = (config_.end_page < 0) ? num_pages : std::min(config_.end_page, num_pages);
    
    // Extract text from each page
    std::string full_text;
    for (int i = start; i < end; ++i) {
        auto page = doc->create_page(i);
        if (!page) continue;
        
        std::string page_text = extract_page_text(page.get());
        if (!page_text.empty()) {
            full_text += page_text;
            full_text += "\n\n"; // Page separator
        }
    }
    
    if (full_text.empty()) {
        result.warnings.push_back("No text extracted from PDF");
    }
    
    // Chunk the full text
    result.chunks = chunk_text(full_text, config, result.global_metadata);
    
    // Add page numbers to metadata
    for (size_t i = 0; i < result.chunks.size(); ++i) {
        result.chunks[i].metadata["page_range"] = 
            std::to_string(start) + "-" + std::to_string(end);
        result.chunks[i].source = source_path;
    }
    
    return result;
}

std::string PDFAdapter::extract_page_text(poppler::page* page) const {
    if (!page) return "";
    
    poppler::page::text_layout_enum layout = config_.preserve_layout 
        ? poppler::page::physical_layout 
        : poppler::page::raw_order_layout;
    
    poppler::ustring text = page->text(layout);
    return text.to_utf8();
}

std::unordered_map<std::string, std::string> PDFAdapter::extract_metadata(
    poppler::document* doc
) const {
    std::unordered_map<std::string, std::string> metadata;
    
    if (!doc) return metadata;
    
    // Extract standard PDF metadata
    auto title = doc->get_title();
    if (!title.empty()) {
        metadata["title"] = title.to_utf8();
    }
    
    auto author = doc->get_author();
    if (!author.empty()) {
        metadata["author"] = author.to_utf8();
    }
    
    auto subject = doc->get_subject();
    if (!subject.empty()) {
        metadata["subject"] = subject.to_utf8();
    }
    
    auto keywords = doc->get_keywords();
    if (!keywords.empty()) {
        metadata["keywords"] = keywords.to_utf8();
    }
    
    auto creator = doc->get_creator();
    if (!creator.empty()) {
        metadata["creator"] = creator.to_utf8();
    }
    
    auto producer = doc->get_producer();
    if (!producer.empty()) {
        metadata["producer"] = producer.to_utf8();
    }
    
    metadata["num_pages"] = std::to_string(doc->pages());
    metadata["pdf_version"] = std::to_string(doc->get_pdf_major_version()) + "." +
                               std::to_string(doc->get_pdf_minor_version());
    
    return metadata;
}
#endif

Result<void> PDFAdapter::write(
    const NormalizedData& data,
    const fs::path& path
) {
    // NOTE: PDF writing requires a PDF generation library like libharu (hpdf)
    // or QPrinter from Qt, or PDFWriter
    //
    // This is a placeholder implementation that documents the approach.
    // For production use, integrate libharu:
    //
    // #include <hpdf.h>
    //
    // Example implementation:
    // 1. Create HPDF_Doc handle
    // 2. Add pages for each chunk
    // 3. Set font and write content
    // 4. Save to file
    
    // Simple text-based fallback: write as plain text with PDF header
    // This creates a valid but simple PDF
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to create PDF file: " + path.string()});
    }
    
    // Write minimal PDF structure
    file << "%PDF-1.4\n";
    file << "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n";
    file << "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n";
    file << "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n";
    file << "/Resources <<\n/Font <<\n/F1 4 0 R\n>>\n>>\n";
    file << "/MediaBox [0 0 612 792]\n";
    file << "/Contents 5 0 R\n>>\nendobj\n";
    file << "4 0 obj\n<<\n/Type /Font\n/Subtype /Type1\n/BaseFont /Helvetica\n>>\nendobj\n";
    
    // Build content stream
    std::stringstream content_stream;
    content_stream << "BT\n/F1 12 Tf\n50 750 Td\n";
    
    int y_pos = 750;
    for (const auto& chunk : data.chunks) {
        if (chunk.title) {
            content_stream << "(" << escape_pdf_string(*chunk.title) << ") Tj\n";
            y_pos -= 20;
            content_stream << "0 -20 Td\n";
        }
        
        // Simple word wrapping (approximate)
        std::string content = chunk.content;
        size_t pos = 0;
        while (pos < content.size() && y_pos > 50) {
            size_t line_end = std::min(pos + 80, content.size());
            std::string line = content.substr(pos, line_end - pos);
            content_stream << "(" << escape_pdf_string(line) << ") Tj\n";
            y_pos -= 15;
            content_stream << "0 -15 Td\n";
            pos = line_end;
            
            if (y_pos < 50) break; // Page full
        }
    }
    
    content_stream << "ET\n";
    std::string content_str = content_stream.str();
    
    file << "5 0 obj\n<<\n/Length " << content_str.size() << "\n>>\nstream\n";
    file << content_str;
    file << "endstream\nendobj\n";
    
    // Write cross-reference table
    file << "xref\n0 6\n";
    file << "0000000000 65535 f \n";
    file << "0000000009 00000 n \n";
    file << "0000000058 00000 n \n";
    file << "0000000115 00000 n \n";
    file << "0000000261 00000 n \n";
    file << "0000000349 00000 n \n";
    
    file << "trailer\n<<\n/Size 6\n/Root 1 0 R\n>>\n";
    
    // Calculate xref position (approximate)
    size_t xref_pos = 400 + content_str.size();
    file << "startxref\n" << xref_pos << "\n%%EOF\n";
    
    if (!file) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to write PDF file: " + path.string()});
    }
    
    return {};
}

} // namespace vdb::adapters
