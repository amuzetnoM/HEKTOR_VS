"""Shared fixtures for competitor analysis tests."""

import pytest
from pathlib import Path


@pytest.fixture(scope="session")
def analysis_path():
    """Get path to competitor analysis document."""
    return Path(__file__).parent.parent / "docs" / "research" / "COMPETITOR_ANALYSIS.md"


@pytest.fixture(scope="session")
def analysis_content(analysis_path):
    """Load competitor analysis content once and reuse."""
    with open(analysis_path, 'r', encoding='utf-8') as f:
        return f.read()


# Common regex patterns for competitor sections
COMPETITOR_SECTION_PATTERN = r"###\s+\d+\.\s+{competitor}.*?{section}"

# Common competitors to test
MAJOR_COMPETITORS = [
    "Pinecone",
    "Weaviate",
    "Milvus",
    "Qdrant",
    "Chroma"
]

ALL_COMPETITORS = MAJOR_COMPETITORS + [
    "Faiss",
    "pgvector",
    "Redis",
    "Elasticsearch"
]
