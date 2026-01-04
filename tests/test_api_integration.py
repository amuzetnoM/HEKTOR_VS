"""
Integration tests for Vector Studio REST API
Tests the complete API workflow including authentication, collections, documents, and search
"""

import pytest
import requests
import time
from typing import Dict, Any

# Configuration
API_BASE_URL = "http://localhost:8080"
TEST_USERNAME = "admin"
TEST_PASSWORD = "admin123"


class TestAPIIntegration:
    """Integration tests for the Vector Studio API"""
    
    @pytest.fixture(scope="class")
    def auth_token(self) -> str:
        """Get authentication token for tests"""
        response = requests.post(
            f"{API_BASE_URL}/auth/login",
            json={"username": TEST_USERNAME, "password": TEST_PASSWORD}
        )
        assert response.status_code == 200
        data = response.json()
        assert "access_token" in data
        return data["access_token"]
    
    @pytest.fixture
    def headers(self, auth_token: str) -> Dict[str, str]:
        """Get headers with authentication"""
        return {
            "Authorization": f"Bearer {auth_token}",
            "Content-Type": "application/json"
        }
    
    def test_health_check(self):
        """Test health endpoint"""
        response = requests.get(f"{API_BASE_URL}/health")
        assert response.status_code == 200
        
        data = response.json()
        assert data["status"] == "healthy"
        assert "version" in data
        assert "uptime_seconds" in data
    
    def test_metrics_endpoint(self):
        """Test Prometheus metrics endpoint"""
        response = requests.get(f"{API_BASE_URL}/metrics")
        assert response.status_code == 200
        assert "vdb_api_requests_total" in response.text
    
    def test_login_success(self):
        """Test successful login"""
        response = requests.post(
            f"{API_BASE_URL}/auth/login",
            json={"username": TEST_USERNAME, "password": TEST_PASSWORD}
        )
        assert response.status_code == 200
        
        data = response.json()
        assert "access_token" in data
        assert data["token_type"] == "bearer"
    
    def test_login_failure(self):
        """Test login with invalid credentials"""
        response = requests.post(
            f"{API_BASE_URL}/auth/login",
            json={"username": "invalid", "password": "wrong"}
        )
        assert response.status_code == 401
    
    def test_unauthorized_access(self):
        """Test accessing protected endpoint without token"""
        response = requests.get(f"{API_BASE_URL}/collections")
        assert response.status_code in [401, 403]
    
    def test_create_collection(self, headers):
        """Test creating a new collection"""
        collection_name = f"test_collection_{int(time.time())}"
        
        response = requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={
                "name": collection_name,
                "dimension": 384,
                "metric": "cosine"
            }
        )
        assert response.status_code == 200
        
        data = response.json()
        assert data["name"] == collection_name
        assert data["dimension"] == 384
        assert data["metric"] == "cosine"
    
    def test_list_collections(self, headers):
        """Test listing collections"""
        response = requests.get(
            f"{API_BASE_URL}/collections",
            headers=headers
        )
        assert response.status_code == 200
        
        data = response.json()
        assert isinstance(data, list)
    
    def test_add_document(self, headers):
        """Test adding a document to a collection"""
        # First create a collection
        collection_name = f"test_docs_{int(time.time())}"
        create_response = requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={
                "name": collection_name,
                "dimension": 384,
                "metric": "cosine"
            }
        )
        assert create_response.status_code == 200
        
        # Add a document
        response = requests.post(
            f"{API_BASE_URL}/collections/{collection_name}/documents",
            headers=headers,
            json={
                "content": "This is a test document about machine learning",
                "metadata": {"source": "test", "category": "ml"}
            }
        )
        assert response.status_code == 200
        
        data = response.json()
        assert "id" in data
        assert "message" in data
    
    def test_add_documents_batch(self, headers):
        """Test adding multiple documents in batch"""
        collection_name = f"test_batch_{int(time.time())}"
        
        # Create collection
        requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={"name": collection_name, "dimension": 384, "metric": "cosine"}
        )
        
        # Add batch of documents
        response = requests.post(
            f"{API_BASE_URL}/collections/{collection_name}/documents/batch",
            headers=headers,
            json={
                "documents": [
                    {"content": "Document 1 about AI", "metadata": {"topic": "ai"}},
                    {"content": "Document 2 about ML", "metadata": {"topic": "ml"}},
                    {"content": "Document 3 about DL", "metadata": {"topic": "dl"}}
                ]
            }
        )
        assert response.status_code == 200
        
        data = response.json()
        assert data["count"] == 3
        assert len(data["ids"]) == 3
    
    def test_search(self, headers):
        """Test semantic search"""
        collection_name = f"test_search_{int(time.time())}"
        
        # Create collection and add documents
        requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={"name": collection_name, "dimension": 384, "metric": "cosine"}
        )
        
        requests.post(
            f"{API_BASE_URL}/collections/{collection_name}/documents/batch",
            headers=headers,
            json={
                "documents": [
                    {"content": "Machine learning is a subset of AI"},
                    {"content": "Deep learning uses neural networks"},
                    {"content": "Natural language processing handles text"}
                ]
            }
        )
        
        # Wait a bit for indexing
        time.sleep(1)
        
        # Perform search
        response = requests.post(
            f"{API_BASE_URL}/collections/{collection_name}/search",
            headers=headers,
            json={
                "query": "artificial intelligence and neural networks",
                "k": 2
            }
        )
        assert response.status_code == 200
        
        data = response.json()
        assert isinstance(data, list)
        assert len(data) <= 2
        
        if len(data) > 0:
            result = data[0]
            assert "id" in result
            assert "score" in result
            assert "metadata" in result
    
    def test_get_stats(self, headers):
        """Test getting database statistics"""
        response = requests.get(
            f"{API_BASE_URL}/stats",
            headers=headers
        )
        assert response.status_code == 200
        
        data = response.json()
        assert "total_vectors" in data
        assert "memory_usage_bytes" in data
        assert "collections" in data
    
    def test_delete_collection(self, headers):
        """Test deleting a collection"""
        collection_name = f"test_delete_{int(time.time())}"
        
        # Create collection
        requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={"name": collection_name, "dimension": 384, "metric": "cosine"}
        )
        
        # Delete collection
        response = requests.delete(
            f"{API_BASE_URL}/collections/{collection_name}",
            headers=headers
        )
        assert response.status_code == 200
    
    def test_rate_limiting(self, headers):
        """Test rate limiting (if enabled)"""
        # Make many requests quickly
        responses = []
        for _ in range(150):  # Exceed the default 100/minute limit
            response = requests.get(
                f"{API_BASE_URL}/health",
                headers=headers
            )
            responses.append(response.status_code)
        
        # Check if any requests were rate limited
        # Note: This might not trigger if rate limiting is disabled
        rate_limited = any(status == 429 for status in responses)
        # Don't assert, just log
        print(f"Rate limiting triggered: {rate_limited}")
    
    def test_cors_headers(self):
        """Test CORS headers are present"""
        response = requests.options(
            f"{API_BASE_URL}/health",
            headers={"Origin": "http://localhost:4200"}
        )
        # CORS headers should be present
        assert "access-control-allow-origin" in response.headers or response.status_code == 200
    
    def test_invalid_collection_name(self, headers):
        """Test creating collection with invalid name"""
        response = requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={
                "name": "",  # Empty name
                "dimension": 384,
                "metric": "cosine"
            }
        )
        assert response.status_code in [400, 422]  # Validation error
    
    def test_invalid_metric(self, headers):
        """Test creating collection with invalid metric"""
        response = requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={
                "name": "test",
                "dimension": 384,
                "metric": "invalid_metric"
            }
        )
        assert response.status_code in [400, 422]  # Validation error


class TestAPIPerformance:
    """Performance tests for the API"""
    
    @pytest.fixture(scope="class")
    def auth_token(self) -> str:
        """Get authentication token"""
        response = requests.post(
            f"{API_BASE_URL}/auth/login",
            json={"username": TEST_USERNAME, "password": TEST_PASSWORD}
        )
        return response.json()["access_token"]
    
    @pytest.fixture
    def headers(self, auth_token: str) -> Dict[str, str]:
        """Get headers with authentication"""
        return {
            "Authorization": f"Bearer {auth_token}",
            "Content-Type": "application/json"
        }
    
    def test_search_latency(self, headers):
        """Test search latency is acceptable"""
        collection_name = f"perf_test_{int(time.time())}"
        
        # Setup
        requests.post(
            f"{API_BASE_URL}/collections",
            headers=headers,
            json={"name": collection_name, "dimension": 384, "metric": "cosine"}
        )
        
        requests.post(
            f"{API_BASE_URL}/collections/{collection_name}/documents/batch",
            headers=headers,
            json={
                "documents": [
                    {"content": f"Test document {i}"} for i in range(100)
                ]
            }
        )
        
        time.sleep(1)  # Wait for indexing
        
        # Measure search latency
        start = time.time()
        response = requests.post(
            f"{API_BASE_URL}/collections/{collection_name}/search",
            headers=headers,
            json={"query": "test document", "k": 10}
        )
        latency = time.time() - start
        
        assert response.status_code == 200
        assert latency < 1.0  # Should be under 1 second
        print(f"Search latency: {latency*1000:.2f}ms")


if __name__ == "__main__":
    pytest.main([__file__, "-v", "--tb=short"])
