// ============================================================================
// VectorDB - Plain Text Data Adapter Implementation
// ============================================================================

#include "vdb/adapters/text_adapter.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <unordered_set>

namespace vdb::adapters {

TextAdapter::TextAdapter(const TextConfig& config) : config_(config) {}

bool TextAdapter::can_handle(const fs::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Common text file extensions
    static const std::unordered_set<std::string> text_exts = {
        ".txt", ".text", ".md", ".markdown", ".rst", ".log",
        ".conf", ".cfg", ".ini", ".yaml", ".yml", ".toml"
    };
    
    return text_exts.count(ext) > 0;
}

bool TextAdapter::can_handle(std::string_view content) const {
    // Check if content appears to be text (UTF-8, ASCII, etc.)
    if (content.empty()) return false;
    
    // Check for binary content indicators
    size_t null_bytes = 0;
    size_t control_chars = 0;
    for (size_t i = 0; i < std::min(content.size(), size_t(1024)); ++i) {
        unsigned char c = content[i];
        if (c == 0) ++null_bytes;
        if (c < 32 && c != '\n' && c != '\r' && c != '\t') ++control_chars;
    }
    
    // If too many null bytes or control chars, probably binary
    return (null_bytes == 0) && (control_chars < content.size() / 20);
}

Result<NormalizedData> TextAdapter::parse(
    const fs::path& path,
    const ChunkConfig& config
) {
    // Read file content
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(Error{
            ErrorCode::IoError,
            "Failed to open text file: " + path.string()
        });
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    return parse_content(content, config, path.string());
}

Result<NormalizedData> TextAdapter::parse_content(
    std::string_view content,
    const ChunkConfig& config,
    std::string_view source_hint
) {
    NormalizedData result;
    result.format = DataFormat::PlainText;
    result.source_path = std::string(source_hint);
    result.confidence = 0.9f;
    
    // Detect and convert encoding if needed
    auto converted_result = detect_and_convert_encoding(content);
    if (!converted_result) {
        return std::unexpected(converted_result.error());
    }
    std::string text = std::move(*converted_result);
    
    // Detect if content is actually markdown or code
    if (config_.detect_markdown && looks_like_markdown(text)) {
        result.format = DataFormat::Markdown;
        result.global_metadata["detected_format"] = "markdown";
    } else if (config_.detect_code && looks_like_code(text)) {
        result.global_metadata["detected_format"] = "code";
        result.global_metadata["preserve_whitespace"] = "true";
    }
    
    // Chunk the text
    result.chunks = chunk_text(text, config, result.global_metadata);
    
    // Add source metadata
    for (auto& chunk : result.chunks) {
        chunk.source = result.source_path;
    }
    
    return result;
}

Result<void> TextAdapter::sanitize(NormalizedData& data) {
    for (auto& chunk : data.chunks) {
        // Only sanitize if not preserving whitespace
        if (data.global_metadata["preserve_whitespace"] != "true") {
            chunk.content = sanitize_text(chunk.content);
        }
    }
    data.sanitized = true;
    return {};
}

Result<std::string> TextAdapter::detect_and_convert_encoding(
    std::string_view content
) const {
    if (!config_.auto_detect_encoding) {
        return std::string(content);
    }
    
#ifdef HAVE_ICU
    std::string detected_encoding = detect_encoding_icu(content);
    
    if (detected_encoding.empty() || detected_encoding == "UTF-8") {
        return std::string(content);
    }
    
    // Convert to UTF-8
    return convert_encoding(content, detected_encoding, "UTF-8");
#else
    // Without ICU, assume UTF-8
    return std::string(content);
#endif
}

bool TextAdapter::looks_like_markdown(std::string_view content) const {
    // Check for markdown indicators
    std::regex md_patterns(
        R"((^|\n)#{1,6}\s|```|\*\*|__|\[.*\]\(.*\)|^[\*\-\+]\s|^\d+\.\s)"
    );
    
    std::string sample(content.substr(0, std::min(content.size(), size_t(2000))));
    return std::regex_search(sample, md_patterns);
}

bool TextAdapter::looks_like_code(std::string_view content) const {
    // Check for code indicators (indentation, braces, semicolons, etc.)
    std::regex code_patterns(R"(\{\s*\n|\}\s*;|^\s{4,}|import\s+|def\s+|class\s+)");
    
    std::string sample(content.substr(0, std::min(content.size(), size_t(2000))));
    return std::regex_search(sample, code_patterns);
}

#ifdef HAVE_ICU
std::string TextAdapter::detect_encoding_icu(std::string_view content) const {
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector* csd = ucsdet_open(&status);
    if (U_FAILURE(status)) return "";
    
    ucsdet_setText(csd, content.data(), content.size(), &status);
    if (U_FAILURE(status)) {
        ucsdet_close(csd);
        return "";
    }
    
    const UCharsetMatch* match = ucsdet_detect(csd, &status);
    std::string encoding;
    if (U_SUCCESS(status) && match) {
        const char* name = ucsdet_getName(match, &status);
        if (U_SUCCESS(status) && name) {
            encoding = name;
        }
    }
    
    ucsdet_close(csd);
    return encoding;
}

Result<std::string> TextAdapter::convert_encoding(
    std::string_view content,
    const std::string& from_encoding,
    const std::string& to_encoding
) const {
    UErrorCode status = U_ZERO_ERROR;
    
    // Open converters
    UConverter* from_conv = ucnv_open(from_encoding.c_str(), &status);
    if (U_FAILURE(status)) {
        return std::unexpected(Error{
            ErrorCode::ParseError,
            "Failed to open source encoding converter: " + from_encoding
        });
    }
    
    UConverter* to_conv = ucnv_open(to_encoding.c_str(), &status);
    if (U_FAILURE(status)) {
        ucnv_close(from_conv);
        return std::unexpected(Error{
            ErrorCode::ParseError,
            "Failed to open target encoding converter: " + to_encoding
        });
    }
    
    // Convert via Unicode
    std::vector<UChar> unicode_buffer(content.size() * 2);
    int32_t unicode_length = ucnv_toUChars(
        from_conv,
        unicode_buffer.data(),
        unicode_buffer.size(),
        content.data(),
        content.size(),
        &status
    );
    
    if (U_FAILURE(status)) {
        ucnv_close(from_conv);
        ucnv_close(to_conv);
        return std::unexpected(Error{
            ErrorCode::ParseError,
            "Failed to convert from " + from_encoding + " to Unicode"
        });
    }
    
    std::vector<char> output_buffer(unicode_length * 4);
    int32_t output_length = ucnv_fromUChars(
        to_conv,
        output_buffer.data(),
        output_buffer.size(),
        unicode_buffer.data(),
        unicode_length,
        &status
    );
    
    ucnv_close(from_conv);
    ucnv_close(to_conv);
    
    if (U_FAILURE(status)) {
        return std::unexpected(Error{
            ErrorCode::ParseError,
            "Failed to convert from Unicode to " + to_encoding
        });
    }
    
    return std::string(output_buffer.data(), output_length);
}
#endif

} // namespace vdb::adapters
