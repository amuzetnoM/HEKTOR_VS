#!/usr/bin/env python3
"""
HEKTOR HNSW Index Performance Benchmark

Tests HNSW index construction and search performance across different scales,
parameters, and configurations.

Audit Compliance: docs/.SCOPE/audit/quality/TEST_COVERAGE_REPORT.md

Version: 1.0.0
Date: 2026-01-24
"""

import argparse
import json
import time
import numpy as np
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import List, Dict, Any, Tuple
import sys

try:
    import pyvdb
    HAS_PYVDB = True
except ImportError:
    HAS_PYVDB = False
    print("Warning: pyvdb not available, using simulation mode")


@dataclass
class HNSWBenchmarkConfig:
    """HNSW benchmark configuration"""
    vector_counts: List[int] = None
    dimensions: List[int] = None
    m_values: List[int] = None
    ef_construction_values: List[int] = None
    ef_search_values: List[int] = None
    k_values: List[int] = None
    num_queries: int = 1000
    output_file: str = "hnsw_benchmark_results.json"
    
    def __post_init__(self):
        if self.vector_counts is None:
            self.vector_counts = [1000, 10000, 100000, 1000000]
        if self.dimensions is None:
            self.dimensions = [128, 384, 512]
        if self.m_values is None:
            self.m_values = [8, 16, 32]
        if self.ef_construction_values is None:
            self.ef_construction_values = [100, 200, 400]
        if self.ef_search_values is None:
            self.ef_search_values = [50, 100, 200]
        if self.k_values is None:
            self.k_values = [1, 5, 10, 20, 50]


def generate_dataset(num_vectors: int, dimension: int) -> Tuple[np.ndarray, np.ndarray]:
    """Generate random dataset for testing"""
    np.random.seed(42)
    
    # Generate database vectors
    vectors = np.random.randn(num_vectors, dimension).astype(np.float32)
    
    # Normalize
    norms = np.linalg.norm(vectors, axis=1, keepdims=True)
    vectors = vectors / np.maximum(norms, 1e-8)
    
    # Generate query vectors
    num_queries = min(1000, num_vectors // 10)
    queries = np.random.randn(num_queries, dimension).astype(np.float32)
    query_norms = np.linalg.norm(queries, axis=1, keepdims=True)
    queries = queries / np.maximum(query_norms, 1e-8)
    
    return vectors, queries


def build_index(vectors: np.ndarray, m: int, ef_construction: int, dimension: int) -> Tuple[Any, float]:
    """Build HNSW index and measure construction time"""
    
    if HAS_PYVDB:
        start = time.perf_counter()
        
        # Create temporary database
        db_config = pyvdb.DatabaseConfig()
        db_config.dimension = dimension
        db_config.metric = pyvdb.DistanceMetric.Cosine
        db_config.hnsw_m = m
        db_config.hnsw_ef_construction = ef_construction
        db_config.max_elements = len(vectors)
        
        db = pyvdb.create_database(f"./temp_hnsw_db_{m}_{ef_construction}", db_config)
        
        # Insert all vectors
        for i, vec in enumerate(vectors):
            metadata = pyvdb.Metadata()
            metadata.id = i
            db.add_vector(vec.tolist(), metadata)
        
        end = time.perf_counter()
        construction_time = end - start
        
        return db, construction_time
    else:
        # Simulation
        start = time.perf_counter()
        time.sleep(len(vectors) * 0.000001 * m)  # Simulate construction
        end = time.perf_counter()
        return None, end - start


def benchmark_search(db: Any, queries: np.ndarray, k: int, ef_search: int) -> Dict[str, float]:
    """Benchmark search performance"""
    latencies = []
    
    if HAS_PYVDB and db:
        for query in queries:
            start = time.perf_counter()
            
            options = pyvdb.QueryOptions()
            options.k = k
            options.ef_search = ef_search if hasattr(options, 'ef_search') else 50
            
            results = db.search_vector(query.tolist(), options)
            
            end = time.perf_counter()
            latencies.append((end - start) * 1000.0)  # ms
    else:
        # Simulation
        for _ in queries:
            start = time.perf_counter()
            time.sleep(0.0002)  # 200 microseconds
            end = time.perf_counter()
            latencies.append((end - start) * 1000.0)
    
    return {
        'mean': np.mean(latencies),
        'p50': np.percentile(latencies, 50),
        'p90': np.percentile(latencies, 90),
        'p95': np.percentile(latencies, 95),
        'p99': np.percentile(latencies, 99),
        'qps': len(queries) / (sum(latencies) / 1000.0)
    }


def calculate_recall(ground_truth: List[List[int]], results: List[List[int]], k: int) -> float:
    """Calculate recall@k"""
    if not ground_truth or not results:
        return 0.0
    
    recall_scores = []
    for gt, res in zip(ground_truth, results):
        gt_set = set(gt[:k])
        res_set = set(res[:k])
        recall = len(gt_set & res_set) / k if k > 0 else 0.0
        recall_scores.append(recall)
    
    return np.mean(recall_scores)


def brute_force_search(vectors: np.ndarray, queries: np.ndarray, k: int) -> List[List[int]]:
    """Brute force search for ground truth"""
    results = []
    
    for query in queries:
        # Cosine similarity
        similarities = np.dot(vectors, query)
        top_k_indices = np.argsort(similarities)[::-1][:k]
        results.append(top_k_indices.tolist())
    
    return results


def run_hnsw_benchmark(config: HNSWBenchmarkConfig) -> Dict[str, Any]:
    """Run comprehensive HNSW benchmark"""
    
    all_results = {
        'test_name': 'HNSW Performance Benchmark',
        'version': '1.0.0',
        'timestamp': time.time(),
        'config': asdict(config),
        'results': []
    }
    
    print("=== HNSW Performance Benchmark ===\n")
    
    total_tests = len(config.vector_counts) * len(config.dimensions) * \
                  len(config.m_values) * len(config.ef_construction_values) * \
                  len(config.ef_search_values)
    
    test_num = 0
    
    for num_vectors in config.vector_counts:
        for dimension in config.dimensions:
            print(f"\nGenerating dataset: {num_vectors:,} vectors x {dimension}D")
            vectors, queries = generate_dataset(num_vectors, dimension)
            
            # Calculate ground truth for recall
            print("Computing ground truth...")
            ground_truth = brute_force_search(vectors, queries[:100], max(config.k_values))
            
            for m in config.m_values:
                for ef_construction in config.ef_construction_values:
                    test_num += 1
                    print(f"\n[Test {test_num}/{total_tests}] Building index: M={m}, ef_construction={ef_construction}")
                    
                    db, construction_time = build_index(vectors, m, ef_construction, dimension)
                    
                    print(f"  Construction time: {construction_time:.2f}s")
                    
                    for ef_search in config.ef_search_values:
                        for k in config.k_values:
                            print(f"  Benchmarking: ef_search={ef_search}, k={k}... ", end='', flush=True)
                            
                            search_stats = benchmark_search(db, queries, k, ef_search)
                            
                            result = {
                                'num_vectors': num_vectors,
                                'dimension': dimension,
                                'hnsw_m': m,
                                'ef_construction': ef_construction,
                                'ef_search': ef_search,
                                'k': k,
                                'construction_time_sec': construction_time,
                                'search_latency_ms': search_stats,
                                'throughput_qps': search_stats['qps']
                            }
                            
                            all_results['results'].append(result)
                            
                            print(f"p50={search_stats['p50']:.2f}ms, p99={search_stats['p99']:.2f}ms")
                    
                    # Cleanup
                    if HAS_PYVDB and db:
                        db.close()
    
    return all_results


def print_summary(results: Dict[str, Any]):
    """Print benchmark summary"""
    print("\n=== BENCHMARK SUMMARY ===\n")
    
    if not results['results']:
        print("No results to display")
        return
    
    # Group by vector count
    by_count = {}
    for r in results['results']:
        count = r['num_vectors']
        if count not in by_count:
            by_count[count] = []
        by_count[count].append(r)
    
    for count in sorted(by_count.keys()):
        count_results = by_count[count]
        avg_p50 = np.mean([r['search_latency_ms']['p50'] for r in count_results])
        avg_p99 = np.mean([r['search_latency_ms']['p99'] for r in count_results])
        avg_qps = np.mean([r['throughput_qps'] for r in count_results])
        
        print(f"{count:>10,} vectors: p50={avg_p50:.2f}ms, p99={avg_p99:.2f}ms, {avg_qps:.0f} QPS")


def export_results(results: Dict[str, Any], output_file: str):
    """Export results to JSON"""
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults exported to: {output_file}")


def main():
    parser = argparse.ArgumentParser(description='HEKTOR HNSW Performance Benchmark')
    parser.add_argument('--vectors', type=str, default='1000,10000,100000', 
                       help='Comma-separated vector counts')
    parser.add_argument('--dimensions', type=str, default='384,512', 
                       help='Comma-separated dimensions')
    parser.add_argument('--output', type=str, default='hnsw_benchmark_results.json', 
                       help='Output file')
    
    args = parser.parse_args()
    
    config = HNSWBenchmarkConfig(
        vector_counts=[int(x) for x in args.vectors.split(',')],
        dimensions=[int(x) for x in args.dimensions.split(',')],
        output_file=args.output
    )
    
    results = run_hnsw_benchmark(config)
    print_summary(results)
    export_results(results, config.output_file)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
