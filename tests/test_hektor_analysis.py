#!/usr/bin/env python3
"""
Tests for Hektor analysis document.
"""

import pytest
import re
from pathlib import Path


class TestHektorAnalysis:
    """Tests for Hektor-specific analysis document."""
    
    @pytest.fixture(scope="class")
    def hektor_doc_path(self):
        """Get path to Hektor analysis document."""
        return Path(__file__).parent.parent / "docs" / "research" / "HEKTOR_ANALYSIS.md"
    
    @pytest.fixture(scope="class")
    def hektor_content(self, hektor_doc_path):
        """Load Hektor analysis content."""
        with open(hektor_doc_path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_document_exists(self, hektor_doc_path):
        """Test that Hektor analysis document exists."""
        assert hektor_doc_path.exists(), "Hektor analysis document must exist"
    
    def test_has_executive_summary(self, hektor_content):
        """Test document has executive summary."""
        assert "Executive Summary" in hektor_content
    
    def test_has_architecture_section(self, hektor_content):
        """Test document has architecture details."""
        assert "Architecture" in hektor_content or "architecture" in hektor_content.lower()
    
    def test_has_performance_benchmarks(self, hektor_content):
        """Test document includes performance benchmarks."""
        assert "Benchmark" in hektor_content or "Performance" in hektor_content
    
    def test_has_quantitative_metrics(self, hektor_content):
        """Test document includes quantitative performance metrics."""
        # Should have latency metrics
        assert re.search(r'\d+\.?\d*\s*ms', hektor_content, re.IGNORECASE)
        # Should have QPS metrics
        assert "QPS" in hektor_content or "queries per second" in hektor_content.lower()
    
    def test_has_simd_optimization_details(self, hektor_content):
        """Test document covers SIMD optimization."""
        assert "SIMD" in hektor_content or "AVX" in hektor_content
    
    def test_has_memory_analysis(self, hektor_content):
        """Test document includes memory analysis."""
        assert "memory" in hektor_content.lower() or "Memory" in hektor_content
    
    def test_has_scalability_section(self, hektor_content):
        """Test document covers scalability."""
        assert "scal" in hektor_content.lower()  # scalability, scaling, etc.
    
    def test_has_configuration_examples(self, hektor_content):
        """Test document includes configuration examples."""
        assert "config" in hektor_content.lower() or "parameter" in hektor_content.lower()
    
    def test_has_api_examples(self, hektor_content):
        """Test document includes API usage examples."""
        assert "API" in hektor_content or "example" in hektor_content.lower()


class TestVisualizationHTML:
    """Tests for competitive comparison HTML visualization."""
    
    @pytest.fixture(scope="class")
    def html_path(self):
        """Get path to HTML visualization."""
        return Path(__file__).parent.parent / "docs" / "research" / "COMPETITIVE_COMPARISON.html"
    
    @pytest.fixture(scope="class")
    def html_content(self, html_path):
        """Load HTML content."""
        with open(html_path, 'r', encoding='utf-8') as f:
            return f.read()
    
    def test_html_exists(self, html_path):
        """Test that HTML visualization exists."""
        assert html_path.exists(), "HTML visualization must exist"
    
    def test_responsive_viewport(self, html_content):
        """Test HTML has responsive viewport meta tag."""
        assert 'name="viewport"' in html_content
        assert 'width=device-width' in html_content
    
    def test_includes_chartjs(self, html_content):
        """Test HTML includes Chart.js library."""
        assert 'chart.js' in html_content.lower()
    
    def test_has_multiple_charts(self, html_content):
        """Test HTML contains multiple chart canvases."""
        canvas_count = html_content.count('<canvas')
        assert canvas_count >= 5, "Should have multiple charts"
    
    def test_responsive_css(self, html_content):
        """Test HTML has responsive CSS."""
        assert '@media' in html_content
        assert 'max-width' in html_content or 'min-width' in html_content
    
    def test_hektor_featured(self, html_content):
        """Test Hektor is featured in the visualization."""
        assert 'Hektor' in html_content or 'hektor' in html_content.lower()
    
    def test_all_competitors_included(self, html_content):
        """Test all major competitors are included."""
        competitors = ['Pinecone', 'Weaviate', 'Milvus', 'Qdrant', 'Chroma']
        for comp in competitors:
            assert comp in html_content, f"{comp} should be in visualization"
    
    def test_no_overflow(self, html_content):
        """Test CSS prevents overflow issues."""
        assert 'overflow' in html_content.lower() or 'box-sizing' in html_content


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
