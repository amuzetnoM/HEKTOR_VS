// ============================================================================
// VectorDB Tests - Ingest Module
// ============================================================================

#include <gtest/gtest.h>
#include "vdb/ingest.hpp"
#include <filesystem>

namespace vdb::test {

namespace fs = std::filesystem;

// ============================================================================
// Document Type Detection
// ============================================================================

TEST(IngestTest, DetectJournalType) {
    auto type = detect_document_type("Journal_2025-12-01.md");
    EXPECT_EQ(type, DocumentType::Journal);
}

TEST(IngestTest, DetectChartType) {
    auto type = detect_document_type("GOLD.png");
    EXPECT_EQ(type, DocumentType::Chart);
}

TEST(IngestTest, DetectCatalystType) {
    auto type = detect_document_type("catalysts_2025-12-01.md");
    EXPECT_EQ(type, DocumentType::CatalystWatchlist);
}

TEST(IngestTest, DetectInstMatrixType) {
    auto type = detect_document_type("inst_matrix_2025-12-01.md");
    EXPECT_EQ(type, DocumentType::InstitutionalMatrix);
}

TEST(IngestTest, Detect3MReportType) {
    auto type = detect_document_type("3m_2025-12-01.md");
    EXPECT_EQ(type, DocumentType::ThreeMonthReport);
}

TEST(IngestTest, Detect1YReportType) {
    auto type = detect_document_type("1y_2025-12-01.md");
    EXPECT_EQ(type, DocumentType::OneYearReport);
}

// ============================================================================
// Date Extraction
// ============================================================================

TEST(IngestTest, ExtractDateFromFilename) {
    auto date = extract_date_from_filename("Journal_2025-12-01.md");
    EXPECT_TRUE(date.has_value());
    EXPECT_EQ(date.value(), "2025-12-01");
}

TEST(IngestTest, ExtractDateFromReport) {
    auto date = extract_date_from_filename("catalysts_2025-12-01.md");
    EXPECT_TRUE(date.has_value());
    EXPECT_EQ(date.value(), "2025-12-01");
}

TEST(IngestTest, NoDateInFilename) {
    auto date = extract_date_from_filename("GOLD.png");
    EXPECT_FALSE(date.has_value());
}

// ============================================================================
// Market Data Extraction
// ============================================================================

TEST(IngestTest, ExtractMarketDataFromHeader) {
    std::string header = "Gold: $4,220.50 | DXY: 103.00 | VIX: 17.00 | 10Y: 4.30%";
    auto data = extract_market_data(header);
    
    EXPECT_TRUE(data.gold_price.has_value());
    EXPECT_NEAR(data.gold_price.value(), 4220.50f, 0.01f);
    
    EXPECT_TRUE(data.dxy.has_value());
    EXPECT_NEAR(data.dxy.value(), 103.00f, 0.01f);
}

TEST(IngestTest, ExtractBias) {
    std::string text = "Bias: BULLISH";
    auto data = extract_market_data(text);
    
    EXPECT_TRUE(data.bias.has_value());
    EXPECT_EQ(data.bias.value(), "BULLISH");
}

// ============================================================================
// Chunking Tests
// ============================================================================

TEST(IngestTest, ChunkShortText) {
    ChunkConfig config;
    config.strategy = ChunkStrategy::FixedSize;
    config.max_chunk_size = 100;
    
    auto chunks = chunk_text("Short text", config);
    
    EXPECT_EQ(chunks.size(), 1);
    EXPECT_EQ(chunks[0], "Short text");
}

TEST(IngestTest, ChunkLongText) {
    ChunkConfig config;
    config.strategy = ChunkStrategy::FixedSize;
    config.max_chunk_size = 50;
    config.overlap = 10;
    
    std::string long_text = "This is a longer text that should be split into multiple chunks for embedding.";
    auto chunks = chunk_text(long_text, config);
    
    EXPECT_GT(chunks.size(), 1);
}

} // namespace vdb::test
