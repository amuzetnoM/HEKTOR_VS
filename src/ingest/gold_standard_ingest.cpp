// ============================================================================
// VectorDB - Gold Standard Ingest Implementation
// ============================================================================

#include "vdb/ingest.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>

// For SHA-256
#include <array>
#include <iomanip>

namespace vdb {

// ============================================================================
// Simple SHA-256 Implementation (for content hashing)
// ============================================================================

namespace {

// SHA-256 constants
constexpr std::array<uint32_t, 64> K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

std::string sha256_impl(std::string_view data) {
    // Initial hash values
    std::array<uint32_t, 8> h = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    // Pre-processing: adding padding bits
    std::vector<uint8_t> msg(data.begin(), data.end());
    uint64_t bit_len = msg.size() * 8;
    
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
        msg.push_back(0x00);
    }
    
    // Append length in bits as big-endian
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xff));
    }
    
    // Process each 512-bit chunk
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        std::array<uint32_t, 64> w{};
        
        // Break chunk into 16 32-bit big-endian words
        for (size_t i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
        }
        
        // Extend the first 16 words into the remaining 48
        for (size_t i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        
        // Initialize working variables
        auto [a, b, c, d, e, f, g, hh] = h;
        
        // Compression
        for (size_t i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            
            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    
    // Produce final hash
    std::ostringstream result;
    for (uint32_t val : h) {
        result << std::hex << std::setfill('0') << std::setw(8) << val;
    }
    return result.str();
}

} // anonymous namespace

std::string content_hash(std::string_view content) {
    return sha256_impl(content);
}

Result<std::string> file_hash(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file"});
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return sha256_impl(buffer.str());
}

// ============================================================================
// Document Type Detection
// ============================================================================

DocumentType detect_document_type(std::string_view filename, std::string_view content) {
    std::string lower_name(filename);
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    // Check filename patterns
    if (lower_name.find("journal") != std::string::npos) {
        return DocumentType::Journal;
    }
    if (lower_name.find("catalyst") != std::string::npos) {
        return DocumentType::CatalystWatchlist;
    }
    if (lower_name.find("inst_matrix") != std::string::npos ||
        lower_name.find("institutional") != std::string::npos) {
        return DocumentType::InstitutionalMatrix;
    }
    if (lower_name.find("economic_calendar") != std::string::npos ||
        lower_name.find("calendar") != std::string::npos) {
        return DocumentType::EconomicCalendar;
    }
    if (lower_name.find("weekly") != std::string::npos) {
        return DocumentType::WeeklyRundown;
    }
    if (lower_name.find("3m") != std::string::npos ||
        lower_name.find("three_month") != std::string::npos) {
        return DocumentType::ThreeMonthReport;
    }
    if (lower_name.find("1y") != std::string::npos ||
        lower_name.find("one_year") != std::string::npos) {
        return DocumentType::OneYearReport;
    }
    if (lower_name.find("monthly_yearly") != std::string::npos) {
        return DocumentType::MonthlyReport;
    }
    if (lower_name.find("premarket") != std::string::npos ||
        lower_name.find("pre_market") != std::string::npos) {
        return DocumentType::PreMarket;
    }
    
    // Check extension for charts
    if (lower_name.ends_with(".png") || lower_name.ends_with(".jpg")) {
        return DocumentType::Chart;
    }
    
    // Check content for clues
    if (!content.empty()) {
        std::string lower_content(content.substr(0, 500));
        std::transform(lower_content.begin(), lower_content.end(), 
                       lower_content.begin(), ::tolower);
        
        if (lower_content.find("catalyst watchlist") != std::string::npos) {
            return DocumentType::CatalystWatchlist;
        }
        if (lower_content.find("institutional scenario") != std::string::npos) {
            return DocumentType::InstitutionalMatrix;
        }
        if (lower_content.find("economic calendar") != std::string::npos) {
            return DocumentType::EconomicCalendar;
        }
    }
    
    return DocumentType::Unknown;
}

std::optional<std::string> extract_date_from_filename(std::string_view filename) {
    // Match YYYY-MM-DD pattern
    std::regex date_regex(R"((\d{4}-\d{2}-\d{2}))");
    std::string fn(filename);
    std::smatch match;
    
    if (std::regex_search(fn, match, date_regex)) {
        return match[1].str();
    }
    
    return std::nullopt;
}

// ============================================================================
// Market Data Extraction
// ============================================================================

MarketData extract_market_data(std::string_view text) {
    MarketData data;
    std::string content(text);
    
    // Gold price: $4,220.50 or Gold: $4,220.50 (case-insensitive)
    std::regex gold_regex(R"(gold[:\s]*\$?([\d,]+\.?\d*))", std::regex::icase);
    std::smatch match;
    if (std::regex_search(content, match, gold_regex)) {
        std::string price = match[1].str();
        price.erase(std::remove(price.begin(), price.end(), ','), price.end());
        data.gold_price = std::stof(price);
    }
    
    // Silver price (case-insensitive)
    std::regex silver_regex(R"(silver[:\s]*\$?([\d,]+\.?\d*))", std::regex::icase);
    if (std::regex_search(content, match, silver_regex)) {
        std::string price = match[1].str();
        price.erase(std::remove(price.begin(), price.end(), ','), price.end());
        data.silver_price = std::stof(price);
    }
    
    // DXY
    std::regex dxy_regex(R"(DXY[:\s]*([\d.]+))");
    if (std::regex_search(content, match, dxy_regex)) {
        data.dxy = std::stof(match[1].str());
    }
    
    // VIX
    std::regex vix_regex(R"(VIX[:\s]*([\d.]+))");
    if (std::regex_search(content, match, vix_regex)) {
        data.vix = std::stof(match[1].str());
    }
    
    // 10Y Yield
    std::regex yield_regex(R"(10Y[:\s]*([\d.]+)%)");
    if (std::regex_search(content, match, yield_regex)) {
        data.yield_10y = std::stof(match[1].str());
    }
    
    // GSR
    std::regex gsr_regex(R"(GSR[:\s]*([\d.]+))");
    if (std::regex_search(content, match, gsr_regex)) {
        data.gsr = std::stof(match[1].str());
    }
    
    // Bias (case-insensitive)
    std::regex bias_regex(R"(bias[:\s]*(\w+))", std::regex::icase);
    if (std::regex_search(content, match, bias_regex)) {
        std::string bias = match[1].str();
        std::transform(bias.begin(), bias.end(), bias.begin(), ::toupper);
        if (bias == "BULLISH" || bias == "BEARISH" || bias == "NEUTRAL") {
            data.bias = bias;
        }
    }
    
    return data;
}

// ============================================================================
// Markdown Parsing
// ============================================================================

Result<ParsedDocument> parse_markdown(std::string_view content, std::string_view filename) {
    ParsedDocument doc;
    doc.content = std::string(content);
    
    // Extract date from filename
    if (!filename.empty()) {
        auto date = extract_date_from_filename(filename);
        if (date) {
            doc.date = *date;
        }
    }
    
    // Detect document type
    doc.type = detect_document_type(filename, content);
    
    // Extract market data from header
    auto market = extract_market_data(content.substr(0, 1000));
    doc.gold_price = market.gold_price;
    doc.silver_price = market.silver_price;
    doc.dxy = market.dxy;
    doc.vix = market.vix;
    doc.yield_10y = market.yield_10y;
    doc.gsr = market.gsr;
    if (market.bias) {
        doc.bias = *market.bias;
    }
    
    // Extract title (first # heading)
    std::regex title_regex(R"(^#\s+(.+)$)", std::regex::multiline);
    std::smatch match;
    std::string content_str(content);
    if (std::regex_search(content_str, match, title_regex)) {
        doc.title = match[1].str();
    }
    
    // Split into sections by ## headings
    std::regex section_regex(R"(##\s+[^\n]+)");
    std::sregex_iterator it(content_str.begin(), content_str.end(), section_regex);
    std::sregex_iterator end;
    
    size_t last_pos = 0;
    while (it != end) {
        if (last_pos > 0) {
            size_t section_end = it->position();
            doc.sections.push_back(content_str.substr(last_pos, section_end - last_pos));
        }
        last_pos = it->position();
        ++it;
    }
    if (last_pos > 0 && last_pos < content_str.size()) {
        doc.sections.push_back(content_str.substr(last_pos));
    }
    
    // Extract chart references
    std::regex chart_regex(R"(\!\[.*?\]\((.*?\.png)\))");
    it = std::sregex_iterator(content_str.begin(), content_str.end(), chart_regex);
    while (it != end) {
        doc.chart_paths.push_back((*it)[1].str());
        ++it;
    }
    
    // Extract executive summary if present
    std::regex summary_regex(R"(##\s*(?:Executive\s+)?Summary\s*\n([\s\S]*?)(?=\n##|\Z))");
    if (std::regex_search(content_str, match, summary_regex)) {
        doc.summary = match[1].str();
        // Trim whitespace
        doc.summary.erase(0, doc.summary.find_first_not_of(" \n\r\t"));
        doc.summary.erase(doc.summary.find_last_not_of(" \n\r\t") + 1);
    }
    
    return doc;
}

// ============================================================================
// Chunking
// ============================================================================

std::vector<std::string> chunk_document(const ParsedDocument& doc, const ChunkConfig& config) {
    std::vector<std::string> chunks;
    
    switch (config.strategy) {
        case ChunkStrategy::None:
            chunks.push_back(doc.content);
            break;
            
        case ChunkStrategy::BySection:
            // Use pre-extracted sections
            if (doc.sections.empty()) {
                chunks.push_back(doc.content);
            } else {
                for (const auto& section : doc.sections) {
                    if (!section.empty()) {
                        chunks.push_back(section);
                    }
                }
            }
            break;
            
        case ChunkStrategy::ByParagraph:
        case ChunkStrategy::FixedSize:
        case ChunkStrategy::Semantic:
            chunks = chunk_text(doc.content, config);
            break;
    }
    
    return chunks;
}

std::vector<std::string> chunk_text(std::string_view text, const ChunkConfig& config) {
    std::vector<std::string> chunks;
    std::string content(text);
    
    switch (config.strategy) {
        case ChunkStrategy::ByParagraph: {
            std::regex para_regex(R"(\n\n+)");
            std::sregex_token_iterator it(content.begin(), content.end(), para_regex, -1);
            std::sregex_token_iterator end;
            while (it != end) {
                std::string para = *it++;
                // Trim
                para.erase(0, para.find_first_not_of(" \n\r\t"));
                para.erase(para.find_last_not_of(" \n\r\t") + 1);
                if (!para.empty()) {
                    chunks.push_back(para);
                }
            }
            break;
        }
        
        case ChunkStrategy::FixedSize: {
            size_t pos = 0;
            while (pos < content.size()) {
                size_t end_pos = std::min(pos + config.max_chunk_size, content.size());
                
                // Try to break at sentence boundary
                if (end_pos < content.size()) {
                    size_t break_pos = content.rfind('.', end_pos);
                    if (break_pos != std::string::npos && break_pos > pos + config.max_chunk_size / 2) {
                        end_pos = break_pos + 1;
                    }
                }
                
                chunks.push_back(content.substr(pos, end_pos - pos));
                pos = end_pos - config.overlap;
                if (pos >= content.size()) break;
            }
            break;
        }
        
        default:
            chunks.push_back(content);
    }
    
    return chunks;
}

// ============================================================================
// Gold Standard Ingest
// ============================================================================

GoldStandardIngest::GoldStandardIngest(VectorDatabase& db)
    : db_(db)
{}

std::vector<fs::path> GoldStandardIngest::discover_files(const IngestConfig& config) {
    std::vector<fs::path> files;
    
    if (!fs::exists(config.gold_standard_output)) {
        return files;
    }
    
    // Journals
    if (config.include_journals) {
        for (const auto& entry : fs::directory_iterator(config.gold_standard_output)) {
            if (entry.path().extension() == ".md" && 
                entry.path().filename().string().find("Journal") != std::string::npos) {
                files.push_back(entry.path());
            }
        }
    }
    
    // Charts
    if (config.include_charts) {
        auto charts_dir = config.gold_standard_output / "charts";
        if (fs::exists(charts_dir)) {
            for (const auto& entry : fs::directory_iterator(charts_dir)) {
                if (entry.path().extension() == ".png") {
                    files.push_back(entry.path());
                }
            }
        }
    }
    
    // Reports
    if (config.include_reports) {
        auto reports_dir = config.gold_standard_output / "reports";
        if (fs::exists(reports_dir)) {
            for (const auto& entry : fs::directory_iterator(reports_dir)) {
                if (entry.path().extension() == ".md") {
                    files.push_back(entry.path());
                }
            }
        }
    }
    
    // Filter by date if specified
    if (config.since_date) {
        files.erase(
            std::remove_if(files.begin(), files.end(), [&](const fs::path& p) {
                auto date = extract_date_from_filename(p.filename().string());
                return date && *date < *config.since_date;
            }),
            files.end()
        );
    }
    
    return files;
}

bool GoldStandardIngest::is_ingested(const fs::path& path) const {
    return ingested_files_.contains(path.string());
}

Metadata GoldStandardIngest::create_metadata(
    const ParsedDocument& doc,
    const fs::path& source_path
) {
    Metadata meta;
    meta.type = doc.type;
    meta.date = doc.date;
    meta.source_file = source_path.string();
    meta.bias = doc.bias;
    meta.gold_price = doc.gold_price;
    meta.silver_price = doc.silver_price;
    meta.gsr = doc.gsr;
    meta.dxy = doc.dxy;
    meta.vix = doc.vix;
    meta.yield_10y = doc.yield_10y;
    meta.content_hash = content_hash(doc.content);
    meta.created_at = now_timestamp();
    meta.updated_at = now_timestamp();
    return meta;
}

Result<VectorId> GoldStandardIngest::ingest_journal(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open journal file"});
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    auto doc_result = parse_markdown(content, path.filename().string());
    if (!doc_result) {
        return std::unexpected(doc_result.error());
    }
    
    auto meta = create_metadata(*doc_result, path);
    meta.type = DocumentType::Journal;
    
    auto result = db_.add_text(content, meta);
    if (result) {
        ingested_files_.insert(path.string());
    }
    return result;
}

Result<VectorId> GoldStandardIngest::ingest_chart(const fs::path& path) {
    Metadata meta;
    meta.type = DocumentType::Chart;
    meta.source_file = path.string();
    
    // Parse chart filename for asset and date
    std::string filename = path.filename().string();
    
    // Extract asset (e.g., "GOLD.png" -> "GOLD")
    auto dot_pos = filename.rfind('.');
    if (dot_pos != std::string::npos) {
        meta.asset = filename.substr(0, dot_pos);
    }
    
    // Try to get date from parent directory name or filename
    auto date = extract_date_from_filename(path.parent_path().string());
    if (!date) {
        date = extract_date_from_filename(filename);
    }
    if (date) {
        meta.date = *date;
    }
    
    meta.created_at = now_timestamp();
    meta.updated_at = now_timestamp();
    
    auto result = db_.add_image(path, meta);
    if (result) {
        ingested_files_.insert(path.string());
    }
    return result;
}

Result<VectorId> GoldStandardIngest::ingest_report(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open report file"});
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    auto doc_result = parse_markdown(content, path.filename().string());
    if (!doc_result) {
        return std::unexpected(doc_result.error());
    }
    
    auto meta = create_metadata(*doc_result, path);
    
    auto result = db_.add_text(content, meta);
    if (result) {
        ingested_files_.insert(path.string());
    }
    return result;
}

Result<IngestStats> GoldStandardIngest::ingest(const IngestConfig& config) {
    IngestStats stats;
    
    auto files = discover_files(config);
    
    for (const auto& file : files) {
        if (config.incremental && is_ingested(file)) {
            stats.skipped++;
            continue;
        }
        
        DocumentType type = detect_document_type(file.filename().string(), "");
        Result<VectorId> result;
        
        try {
            switch (type) {
                case DocumentType::Journal:
                    result = ingest_journal(file);
                    if (result) stats.journals_added++;
                    break;
                    
                case DocumentType::Chart:
                    result = ingest_chart(file);
                    if (result) stats.charts_added++;
                    break;
                    
                default:
                    result = ingest_report(file);
                    if (result) stats.reports_added++;
                    break;
            }
            
            if (!result) {
                stats.errors++;
                stats.error_messages.push_back(
                    file.string() + ": " + result.error().message
                );
            }
        } catch (const std::exception& e) {
            stats.errors++;
            stats.error_messages.push_back(
                file.string() + ": " + e.what()
            );
        }
    }
    
    last_stats_ = stats;
    return stats;
}

Result<void> GoldStandardIngest::watch(
    const IngestConfig& config,
    std::function<bool()> should_stop
) {
    while (!should_stop()) {
        auto result = ingest(config);
        if (!result) {
            return std::unexpected(result.error());
        }
        
        // Sleep for a bit before checking again
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    
    return {};
}

} // namespace vdb
