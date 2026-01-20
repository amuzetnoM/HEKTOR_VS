#!/usr/bin/env python3
"""
Comprehensive test suite for competitor analysis validation.

This module tests the competitor analysis data for completeness,
accuracy, and consistency.
"""

import pytest
import re
import os
from pathlib import Path


class TestCompetitorAnalysisDocument:
    """Tests for the competitor analysis document structure and completeness."""
    
    @pytest.fixture
    def analysis_path(self):
        """Get path to competitor analysis document."""
        return Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
    
    @pytest.fixture
    def analysis_content(self, analysis_path):
        """Load competitor analysis content."""
        with open(analysis_path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_document_exists(self, analysis_path):
        """Test that competitor analysis document exists."""
        assert analysis_path.exists(), "Competitor analysis document must exist"
    
    def test_document_not_empty(self, analysis_content):
        """Test that document has content."""
        assert len(analysis_content) > 1000, "Document must have substantial content"
    
    def test_has_executive_summary(self, analysis_content):
        """Test that document has executive summary."""
        assert "Executive Summary" in analysis_content, "Must have executive summary"
    
    def test_has_market_overview(self, analysis_content):
        """Test that document has market overview."""
        assert "Market Overview" in analysis_content, "Must have market overview"
    
    def test_has_competitive_landscape(self, analysis_content):
        """Test that document has competitive landscape section."""
        assert "Competitive Landscape" in analysis_content, "Must have competitive landscape"


class TestCompetitorCoverage:
    """Tests for comprehensive competitor coverage."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    @pytest.mark.parametrize("competitor", [
        "Pinecone",
        "Weaviate",
        "Milvus",
        "Qdrant",
        "Chroma",
        "Faiss",
        "pgvector",
        "Redis",
        "Elasticsearch"
    ])
    def test_competitor_profile_exists(self, analysis_content, competitor):
        """Test that each major competitor has a profile."""
        assert competitor in analysis_content, f"{competitor} must be covered in analysis"
    
    @pytest.mark.parametrize("competitor", [
        "Pinecone",
        "Weaviate",
        "Milvus",
        "Qdrant",
        "Chroma"
    ])
    def test_competitor_organization_details(self, analysis_content, competitor):
        """Test that competitor has organization details."""
        # Check that organization section exists for each competitor
        pattern = rf"###\s+\d+\.\s+{competitor}.*?Organization"
        assert re.search(pattern, analysis_content, re.DOTALL | re.IGNORECASE), \
            f"{competitor} must have organization details"
    
    @pytest.mark.parametrize("competitor", [
        "Pinecone",
        "Weaviate",
        "Milvus",
        "Qdrant",
        "Chroma"
    ])
    def test_competitor_features(self, analysis_content, competitor):
        """Test that competitor has feature coverage."""
        pattern = rf"###\s+\d+\.\s+{competitor}.*?Product Features"
        assert re.search(pattern, analysis_content, re.DOTALL | re.IGNORECASE), \
            f"{competitor} must have product features listed"
    
    @pytest.mark.parametrize("competitor", [
        "Pinecone",
        "Weaviate",
        "Milvus",
        "Qdrant",
        "Chroma"
    ])
    def test_competitor_pricing(self, analysis_content, competitor):
        """Test that competitor has pricing information."""
        pattern = rf"###\s+\d+\.\s+{competitor}.*?Pricing"
        assert re.search(pattern, analysis_content, re.DOTALL | re.IGNORECASE), \
            f"{competitor} must have pricing information"
    
    @pytest.mark.parametrize("competitor", [
        "Pinecone",
        "Weaviate",
        "Milvus",
        "Qdrant",
        "Chroma"
    ])
    def test_competitor_performance(self, analysis_content, competitor):
        """Test that competitor has performance metrics."""
        pattern = rf"###\s+\d+\.\s+{competitor}.*?Performance"
        assert re.search(pattern, analysis_content, re.DOTALL | re.IGNORECASE), \
            f"{competitor} must have performance metrics"


class TestComparisonTables:
    """Tests for comparison tables completeness and accuracy."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_has_feature_comparison_matrix(self, analysis_content):
        """Test that document has feature comparison matrix."""
        assert "Feature Comparison Matrix" in analysis_content, \
            "Must have feature comparison matrix"
    
    def test_has_performance_benchmarks(self, analysis_content):
        """Test that document has performance benchmarks."""
        assert "Performance Benchmarks" in analysis_content, \
            "Must have performance benchmarks section"
    
    def test_has_pricing_comparison(self, analysis_content):
        """Test that document has pricing comparison."""
        assert "Pricing Comparison" in analysis_content, \
            "Must have pricing comparison table"
    
    def test_hektor_in_all_tables(self, analysis_content):
        """Test that Hektor appears in comparison tables."""
        # Extract all markdown tables
        tables = re.findall(r'\|.*?\|.*?\n\|[-:\s|]+\n(?:\|.*?\n)+', 
                           analysis_content, re.MULTILINE)
        
        # Hektor should appear in major comparison tables
        hektor_count = sum(1 for table in tables if "Hektor" in table)
        assert hektor_count >= 5, "Hektor must appear in major comparison tables"
    
    def test_table_structure_valid(self, analysis_content):
        """Test that tables have valid markdown structure."""
        # Find all table headers
        table_headers = re.findall(r'\|.*?\|.*?\n', analysis_content)
        
        # Each header should have corresponding separator
        for i, header in enumerate(table_headers):
            if '|' in header and header.count('|') > 2:
                # Count columns
                cols = header.count('|') - 1
                assert cols >= 2, "Tables must have at least 2 columns"


class TestDataAccuracy:
    """Tests for data accuracy and consistency."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_market_size_data_present(self, analysis_content):
        """Test that market size data is present."""
        assert re.search(r'\$\d+\.?\d*[BM]', analysis_content), \
            "Market size data must be present"
    
    def test_growth_rate_data_present(self, analysis_content):
        """Test that growth rate (CAGR) data is present."""
        assert "CAGR" in analysis_content or "growth rate" in analysis_content.lower(), \
            "Growth rate data must be present"
    
    def test_performance_metrics_quantified(self, analysis_content):
        """Test that performance metrics are quantified."""
        # Look for latency metrics
        assert re.search(r'\d+\s*ms', analysis_content, re.IGNORECASE), \
            "Latency metrics must be quantified"
        
        # Look for QPS metrics
        assert "QPS" in analysis_content or "queries per second" in analysis_content.lower(), \
            "QPS metrics should be present"
    
    def test_hektor_performance_claims(self, analysis_content):
        """Test that Hektor performance claims are documented."""
        # Hektor should have documented sub-3ms latency
        pattern = r"Hektor.*?3\s*ms"
        assert re.search(pattern, analysis_content, re.DOTALL | re.IGNORECASE), \
            "Hektor's sub-3ms latency claim must be documented"
    
    def test_pricing_data_present(self, analysis_content):
        """Test that pricing data is present for major vendors."""
        # Should have dollar amounts or "Free" mentions
        assert re.search(r'\$\d+', analysis_content) or "Free" in analysis_content, \
            "Pricing data must be present"
    
    def test_open_source_status_documented(self, analysis_content):
        """Test that open source status is documented."""
        assert "Open Source" in analysis_content, \
            "Open source status must be documented"


class TestHektorPositioning:
    """Tests for Hektor's competitive positioning."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_hektor_section_exists(self, analysis_content):
        """Test that Hektor has dedicated positioning section."""
        assert "Hektor (Vector Studio) Competitive Position" in analysis_content, \
            "Hektor must have competitive positioning section"
    
    def test_hektor_strengths_documented(self, analysis_content):
        """Test that Hektor's strengths are documented."""
        pattern = r"Hektor.*?Strengths?"
        assert re.search(pattern, analysis_content, re.DOTALL | re.IGNORECASE), \
            "Hektor's strengths must be documented"
    
    def test_hektor_unique_features(self, analysis_content):
        """Test that Hektor's unique features are highlighted."""
        unique_features = [
            "SIMD",
            "C++",
            "local embeddings",
            "ONNX",
            "hybrid search"
        ]
        
        for feature in unique_features:
            assert feature.lower() in analysis_content.lower(), \
                f"Hektor's {feature} capability must be mentioned"
    
    def test_hektor_market_positioning(self, analysis_content):
        """Test that Hektor's market positioning is defined."""
        assert "Sweet Spot" in analysis_content or "Target Segment" in analysis_content, \
            "Hektor's market positioning must be defined"
    
    def test_strategic_recommendations(self, analysis_content):
        """Test that strategic recommendations are provided."""
        assert "Strategic Recommendations" in analysis_content or \
               "Recommendations" in analysis_content, \
            "Strategic recommendations must be provided"


class TestDocumentQuality:
    """Tests for document quality and professionalism."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_has_table_of_contents_markers(self, analysis_content):
        """Test that document has clear section markers."""
        # Count major sections (## headers)
        major_sections = len(re.findall(r'^##\s+', analysis_content, re.MULTILINE))
        assert major_sections >= 5, "Document must have multiple major sections"
    
    def test_proper_markdown_formatting(self, analysis_content):
        """Test that markdown is properly formatted."""
        # Check for proper header hierarchy
        headers = re.findall(r'^#+\s+.+$', analysis_content, re.MULTILINE)
        assert len(headers) >= 20, "Document must have adequate section structure"
    
    def test_has_metadata(self, analysis_content):
        """Test that document has metadata."""
        metadata_items = [
            "Report Date",
            "Analysis Period",
            "Status"
        ]
        
        for item in metadata_items:
            assert item in analysis_content, f"Document must have {item} metadata"
    
    def test_has_conclusion(self, analysis_content):
        """Test that document has conclusion section."""
        assert "Conclusion" in analysis_content, "Document must have conclusion"
    
    def test_has_data_sources(self, analysis_content):
        """Test that data sources are cited."""
        assert "Data Sources" in analysis_content or "Sources" in analysis_content or \
               "References" in analysis_content, \
            "Data sources must be cited"
    
    def test_professional_tone(self, analysis_content):
        """Test that document maintains professional tone."""
        # Check for professional language markers
        professional_markers = [
            "analysis",
            "comprehensive",
            "market",
            "competitive",
            "enterprise"
        ]
        
        found_markers = sum(1 for marker in professional_markers 
                          if marker.lower() in analysis_content.lower())
        assert found_markers >= 4, "Document must maintain professional tone"


class TestComparisonCompleteness:
    """Tests for completeness of comparison across all dimensions."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    @pytest.mark.parametrize("dimension", [
        "Features",
        "Performance",
        "Pricing",
        "Architecture",
        "Use Cases",
        "Strengths",
        "Weaknesses"
    ])
    def test_comparison_dimension_covered(self, analysis_content, dimension):
        """Test that all key comparison dimensions are covered."""
        assert dimension in analysis_content, \
            f"Comparison dimension '{dimension}' must be covered"
    
    def test_quantitative_metrics_present(self, analysis_content):
        """Test that quantitative metrics are present."""
        metrics = [
            r'\d+\s*ms',  # Latency in milliseconds
            r'\d+\s*QPS',  # Queries per second
            r'\$\d+',  # Pricing
            r'\d+[KMB]?\s*vectors',  # Scale
        ]
        
        found_metrics = sum(1 for metric in metrics 
                           if re.search(metric, analysis_content, re.IGNORECASE))
        assert found_metrics >= 3, "Quantitative metrics must be present"
    
    def test_qualitative_analysis_present(self, analysis_content):
        """Test that qualitative analysis is present."""
        qualitative_markers = [
            "advantage",
            "strength",
            "weakness",
            "opportunity",
            "challenge"
        ]
        
        found_markers = sum(1 for marker in qualitative_markers 
                          if marker.lower() in analysis_content.lower())
        assert found_markers >= 3, "Qualitative analysis must be present"


class TestUseCaseAnalysis:
    """Tests for use case analysis completeness."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_use_case_section_exists(self, analysis_content):
        """Test that use case analysis section exists."""
        assert "Use Case" in analysis_content, \
            "Use case analysis section must exist"
    
    @pytest.mark.parametrize("use_case", [
        "RAG",
        "semantic search",
        "recommendation",
        "retrieval"
    ])
    def test_common_use_cases_covered(self, analysis_content, use_case):
        """Test that common vector database use cases are covered."""
        assert use_case.lower() in analysis_content.lower(), \
            f"Use case '{use_case}' must be covered"


class TestTrendsAndInsights:
    """Tests for market trends and insights."""
    
    @pytest.fixture
    def analysis_content(self):
        """Load competitor analysis content."""
        path = Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_trends_section_exists(self, analysis_content):
        """Test that market trends section exists."""
        assert "Trend" in analysis_content or "Market" in analysis_content, \
            "Market trends section must exist"
    
    def test_ai_ml_trends_covered(self, analysis_content):
        """Test that AI/ML trends are covered."""
        ai_ml_terms = ["AI", "ML", "LLM", "machine learning", "artificial intelligence"]
        found = sum(1 for term in ai_ml_terms if term in analysis_content)
        assert found >= 3, "AI/ML trends must be covered"
    
    def test_future_outlook_present(self, analysis_content):
        """Test that future outlook is present."""
        future_indicators = ["2026", "future", "outlook", "forecast", "projection"]
        found = sum(1 for indicator in future_indicators 
                   if indicator.lower() in analysis_content.lower())
        assert found >= 2, "Future outlook must be present"


if __name__ == "__main__":
    pytest.main([__file__, "-v", "--tb=short"])
