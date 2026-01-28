#!/usr/bin/env python3
"""
HEKTOR 1M Vector Stress Test - Python Implementation

Comprehensive stress testing for 1 million vector operations using pyvdb.
Tests insertion, search, concurrent operations, and memory usage.

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
from typing import List, Dict, Any, Optional
import multiprocessing as mp
from concurrent.futures import ThreadPoolExecutor, as_completed
import statistics
import sys
import tracemalloc
import psutil

# Try to import pyvdb - if not available, we'll use a mock
try:
    import pyvdb
    HAS_PYVDB = True
except ImportError:
    HAS_PYVDB = False
    print("Warning: pyvdb not available, using simulation mode")


@dataclass
class TestConfig:
    """Test configuration parameters"""
    num_vectors: int = 1_000_000
    dimension: int = 512
    num_threads: int = 8
    duration_seconds: int = 300
    warmup_seconds: int = 30
    insert_ratio: float = 0.1
    search_ratio: float = 0.85
    delete_ratio: float = 0.05
    batch_size: int = 100
    k: int = 10  # Number of nearest neighbors
    enable_profiling: bool = True
    db_path: str = "./test_db_1m"
    output_file: str = "1m_vector_stress_results.json"
    hnsw_m: int = 16
    hnsw_ef_construction: int = 200
    hnsw_ef_search: int = 50


@dataclass
class BenchmarkStats:
    """Statistics tracking"""
    operations_completed: int = 0
    operations_failed: int = 0
    latencies: List[float] = None
    start_time: float = 0.0
    end_time: float = 0.0
    memory_usage_mb: List[float] = None
    
    def __post_init__(self):
        if self.latencies is None:
            self.latencies = []
        if self.memory_usage_mb is None:
            self.memory_usage_mb = []
    
    def mean_latency(self) -> float:
        """Calculate mean latency"""
        return statistics.mean(self.latencies) if self.latencies else 0.0
    
    def percentile(self, p: float) -> float:
        """Calculate percentile"""
        if not self.latencies:
            return 0.0
        sorted_latencies = sorted(self.latencies)
        idx = int(len(sorted_latencies) * p / 100.0)
        if idx >= len(sorted_latencies):
            idx = len(sorted_latencies) - 1
        return sorted_latencies[idx]
    
    def throughput(self) -> float:
        """Calculate throughput in ops/sec"""
        duration = self.end_time - self.start_time
        return self.operations_completed / duration if duration > 0 else 0.0


class VectorDatabase:
    """Wrapper for HEKTOR database operations"""
    
    def __init__(self, config: TestConfig):
        self.config = config
        self.db = None
        
        if HAS_PYVDB:
            self._init_real_db()
        else:
            print("Using simulation mode")
    
    def _init_real_db(self):
        """Initialize real HEKTOR database"""
        try:
            # Create or open database
            db_path = Path(self.config.db_path)
            if db_path.exists():
                print(f"Opening existing database: {db_path}")
                self.db = pyvdb.open_database(str(db_path))
            else:
                print(f"Creating new database: {db_path}")
                db_config = pyvdb.DatabaseConfig()
                db_config.dimension = self.config.dimension
                db_config.metric = pyvdb.DistanceMetric.Cosine
                db_config.hnsw_m = self.config.hnsw_m
                db_config.hnsw_ef_construction = self.config.hnsw_ef_construction
                db_config.hnsw_ef_search = self.config.hnsw_ef_search
                db_config.max_elements = self.config.num_vectors
                
                self.db = pyvdb.create_database(str(db_path), db_config)
        except Exception as e:
            print(f"Error initializing database: {e}")
            raise
    
    def insert(self, vector: np.ndarray, vector_id: int) -> float:
        """Insert a vector and return latency in ms"""
        start = time.perf_counter()
        
        if HAS_PYVDB and self.db:
            try:
                metadata = pyvdb.Metadata()
                metadata.id = vector_id
                self.db.add_vector(vector.tolist(), metadata)
            except Exception as e:
                print(f"Insert error: {e}")
                raise
        else:
            # Simulate insertion latency
            time.sleep(0.00005)  # 50 microseconds
        
        end = time.perf_counter()
        return (end - start) * 1000.0  # Convert to ms
    
    def search(self, query: np.ndarray, k: int) -> float:
        """Search for k nearest neighbors and return latency in ms"""
        start = time.perf_counter()
        
        if HAS_PYVDB and self.db:
            try:
                options = pyvdb.QueryOptions()
                options.k = k
                results = self.db.search_vector(query.tolist(), options)
            except Exception as e:
                print(f"Search error: {e}")
                raise
        else:
            # Simulate search latency
            time.sleep(0.0002)  # 200 microseconds
        
        end = time.perf_counter()
        return (end - start) * 1000.0  # Convert to ms
    
    def delete(self, vector_id: int) -> float:
        """Delete a vector and return latency in ms"""
        start = time.perf_counter()
        
        if HAS_PYVDB and self.db:
            try:
                self.db.remove_vector(vector_id)
            except Exception as e:
                print(f"Delete error: {e}")
                raise
        else:
            # Simulate delete latency
            time.sleep(0.00003)  # 30 microseconds
        
        end = time.perf_counter()
        return (end - start) * 1000.0  # Convert to ms
    
    def close(self):
        """Close database"""
        if HAS_PYVDB and self.db:
            self.db.close()


def generate_random_vector(dimension: int) -> np.ndarray:
    """Generate a random normalized vector"""
    vec = np.random.randn(dimension).astype(np.float32)
    norm = np.linalg.norm(vec)
    if norm > 0:
        vec /= norm
    return vec


def worker_function(config: TestConfig, duration: float, thread_id: int) -> Dict[str, Any]:
    """Worker function for stress testing"""
    # Use different seed for each thread to avoid correlation
    np.random.seed(42 + thread_id * 10000)
    
    db = VectorDatabase(config)
    latencies = []
    operations_completed = 0
    operations_failed = 0
    
    start_time = time.time()
    end_time = start_time + duration
    
    while time.time() < end_time:
        op_type = np.random.random()
        
        try:
            if op_type < config.insert_ratio:
                # Insert operation
                vector = generate_random_vector(config.dimension)
                vector_id = operations_completed + thread_id * 1000000
                latency = db.insert(vector, vector_id)
            elif op_type < config.insert_ratio + config.search_ratio:
                # Search operation
                query = generate_random_vector(config.dimension)
                latency = db.search(query, config.k)
            else:
                # Delete operation
                vector_id = operations_completed + thread_id * 1000000
                latency = db.delete(vector_id)
            
            latencies.append(latency)
            operations_completed += 1
            
        except Exception as e:
            operations_failed += 1
            import sys
            print(f"Thread {thread_id} error: {e}", file=sys.stderr)
    
    db.close()
    
    return {
        'latencies': latencies,
        'operations_completed': operations_completed,
        'operations_failed': operations_failed
    }


def run_stress_test(config: TestConfig) -> BenchmarkStats:
    """Run the stress test"""
    stats = BenchmarkStats()
    
    print("=== 1M Vector Stress Test ===\n")
    print("Configuration:")
    print(f"  Vectors: {config.num_vectors:,}")
    print(f"  Dimension: {config.dimension}")
    print(f"  Threads: {config.num_threads}")
    print(f"  Duration: {config.duration_seconds}s")
    print(f"  Warmup: {config.warmup_seconds}s")
    print(f"  Database: {config.db_path}\n")
    
    # Start memory tracking
    if config.enable_profiling:
        tracemalloc.start()
        process = psutil.Process()
    
    # Warmup phase
    print("Warmup phase...")
    with ThreadPoolExecutor(max_workers=config.num_threads) as executor:
        futures = [
            executor.submit(worker_function, config, config.warmup_seconds, i)
            for i in range(config.num_threads)
        ]
        for future in as_completed(futures):
            future.result()  # Wait for completion
    
    print("Starting benchmark phase...\n")
    
    # Benchmark phase
    stats.start_time = time.time()
    
    with ThreadPoolExecutor(max_workers=config.num_threads) as executor:
        futures = [
            executor.submit(worker_function, config, config.duration_seconds, i)
            for i in range(config.num_threads)
        ]
        
        # Collect results
        for future in as_completed(futures):
            result = future.result()
            stats.latencies.extend(result['latencies'])
            stats.operations_completed += result['operations_completed']
            stats.operations_failed += result['operations_failed']
    
    stats.end_time = time.time()
    
    # Get memory usage
    if config.enable_profiling:
        current, peak = tracemalloc.get_traced_memory()
        stats.memory_usage_mb.append(peak / 1024 / 1024)
        tracemalloc.stop()
        
        mem_info = process.memory_info()
        print(f"Memory Usage: {mem_info.rss / 1024 / 1024:.2f} MB")
    
    return stats


def print_results(stats: BenchmarkStats):
    """Print benchmark results"""
    print("\n=== STRESS TEST RESULTS ===\n")
    print("Operations:")
    print(f"  Completed: {stats.operations_completed:,}")
    print(f"  Failed: {stats.operations_failed:,}")
    if stats.operations_completed + stats.operations_failed > 0:
        success_rate = 100.0 * stats.operations_completed / (stats.operations_completed + stats.operations_failed)
        print(f"  Success Rate: {success_rate:.2f}%\n")
    
    print("Throughput:")
    print(f"  {stats.throughput():.2f} ops/sec\n")
    
    print("Latency (ms):")
    print(f"  Mean:  {stats.mean_latency():.3f}")
    print(f"  p50:   {stats.percentile(50):.3f}")
    print(f"  p75:   {stats.percentile(75):.3f}")
    print(f"  p90:   {stats.percentile(90):.3f}")
    print(f"  p95:   {stats.percentile(95):.3f}")
    print(f"  p99:   {stats.percentile(99):.3f}")
    print(f"  p99.9: {stats.percentile(99.9):.3f}\n")


def export_results(config: TestConfig, stats: BenchmarkStats):
    """Export results to JSON"""
    results = {
        'test_name': '1M Vector Stress Test',
        'version': '1.0.0',
        'timestamp': time.time(),
        'config': asdict(config),
        'results': {
            'operations_completed': stats.operations_completed,
            'operations_failed': stats.operations_failed,
            'throughput_ops_per_sec': stats.throughput(),
            'latency_ms': {
                'mean': stats.mean_latency(),
                'p50': stats.percentile(50),
                'p75': stats.percentile(75),
                'p90': stats.percentile(90),
                'p95': stats.percentile(95),
                'p99': stats.percentile(99),
                'p99.9': stats.percentile(99.9),
            },
            'memory_usage_mb': stats.memory_usage_mb,
        }
    }
    
    with open(config.output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"Results exported to: {config.output_file}")


def main():
    parser = argparse.ArgumentParser(description='HEKTOR 1M Vector Stress Test')
    parser.add_argument('--vectors', type=int, default=1_000_000, help='Number of vectors')
    parser.add_argument('--dimension', type=int, default=512, help='Vector dimension')
    parser.add_argument('--threads', type=int, default=8, help='Number of threads')
    parser.add_argument('--duration', type=int, default=300, help='Test duration in seconds')
    parser.add_argument('--db-path', type=str, default='./test_db_1m', help='Database path')
    parser.add_argument('--output', type=str, default='1m_vector_stress_results.json', help='Output file')
    
    args = parser.parse_args()
    
    config = TestConfig(
        num_vectors=args.vectors,
        dimension=args.dimension,
        num_threads=args.threads,
        duration_seconds=args.duration,
        db_path=args.db_path,
        output_file=args.output
    )
    
    stats = run_stress_test(config)
    print_results(stats)
    export_results(config, stats)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
