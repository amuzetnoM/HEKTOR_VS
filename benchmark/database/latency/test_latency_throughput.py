#!/usr/bin/env python3
"""
HEKTOR Latency & Throughput Comprehensive Benchmark

Measures detailed latency percentiles and throughput characteristics
across various load conditions and configurations.

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
from typing import List, Dict, Any
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed

try:
    import pyvdb
    HAS_PYVDB = True
except ImportError:
    HAS_PYVDB = False
    print("Warning: pyvdb not available, using simulation mode")


@dataclass
class LatencyBenchmarkConfig:
    """Latency benchmark configuration"""
    vector_count: int = 100000
    dimension: int = 512
    num_queries: int = 10000
    warmup_queries: int = 1000
    batch_sizes: List[int] = None
    concurrent_clients: List[int] = None
    target_qps: List[int] = None
    duration_seconds: int = 60
    percentiles: List[float] = None
    db_path: str = "./test_db_latency"
    output_file: str = "latency_throughput_results.json"
    
    def __post_init__(self):
        if self.batch_sizes is None:
            self.batch_sizes = [1, 10, 50, 100]
        if self.concurrent_clients is None:
            self.concurrent_clients = [1, 4, 8, 16, 32]
        if self.target_qps is None:
            self.target_qps = [100, 500, 1000, 5000]
        if self.percentiles is None:
            self.percentiles = [50, 75, 90, 95, 99, 99.9, 99.99]


class LatencyBenchmark:
    """Latency and throughput benchmark suite"""
    
    def __init__(self, config: LatencyBenchmarkConfig):
        self.config = config
        self.db = None
        self.queries = None
        
    def setup(self):
        """Setup database and queries"""
        print("Setting up database...")
        
        # Generate test data
        np.random.seed(42)
        vectors = np.random.randn(self.config.vector_count, self.config.dimension).astype(np.float32)
        norms = np.linalg.norm(vectors, axis=1, keepdims=True)
        vectors = vectors / np.maximum(norms, 1e-8)
        
        # Generate query vectors
        self.queries = np.random.randn(self.config.num_queries + self.config.warmup_queries, 
                                      self.config.dimension).astype(np.float32)
        query_norms = np.linalg.norm(self.queries, axis=1, keepdims=True)
        self.queries = self.queries / np.maximum(query_norms, 1e-8)
        
        if HAS_PYVDB:
            # Create database
            db_path = Path(self.config.db_path)
            db_config = pyvdb.DatabaseConfig()
            db_config.dimension = self.config.dimension
            db_config.metric = pyvdb.DistanceMetric.Cosine
            db_config.max_elements = self.config.vector_count
            
            self.db = pyvdb.create_database(str(db_path), db_config)
            
            # Insert vectors
            print(f"Inserting {len(vectors):,} vectors...")
            for i, vec in enumerate(vectors):
                if (i + 1) % 10000 == 0:
                    print(f"  Inserted {i + 1:,} vectors...")
                metadata = pyvdb.Metadata()
                metadata.id = i
                self.db.add_vector(vec.tolist(), metadata)
        
        print("Setup completed")
    
    def warmup(self):
        """Warmup phase"""
        print("Warming up...")
        warmup_queries = self.queries[:self.config.warmup_queries]
        
        for query in warmup_queries:
            if HAS_PYVDB and self.db:
                options = pyvdb.QueryOptions()
                options.k = 10
                self.db.search_vector(query.tolist(), options)
            else:
                time.sleep(0.0002)  # Simulate
    
    def measure_latency_distribution(self) -> Dict[str, Any]:
        """Measure latency distribution"""
        print("\n[1/4] Measuring latency distribution...")
        
        queries = self.queries[self.config.warmup_queries:]
        latencies = []
        
        for i, query in enumerate(queries):
            if (i + 1) % 1000 == 0:
                print(f"  Processed {i + 1:,}/{len(queries):,} queries...")
            
            start = time.perf_counter()
            
            if HAS_PYVDB and self.db:
                options = pyvdb.QueryOptions()
                options.k = 10
                self.db.search_vector(query.tolist(), options)
            else:
                time.sleep(0.0002)  # Simulate
            
            end = time.perf_counter()
            latencies.append((end - start) * 1000.0)  # Convert to ms
        
        # Calculate percentiles
        result = {
            'num_queries': len(latencies),
            'mean_ms': float(np.mean(latencies)),
            'std_ms': float(np.std(latencies)),
            'min_ms': float(np.min(latencies)),
            'max_ms': float(np.max(latencies)),
            'percentiles': {}
        }
        
        for p in self.config.percentiles:
            result['percentiles'][f'p{p}'] = float(np.percentile(latencies, p))
        
        print(f"  Mean latency: {result['mean_ms']:.3f}ms")
        print(f"  p50: {result['percentiles']['p50']:.3f}ms")
        print(f"  p99: {result['percentiles']['p99']:.3f}ms")
        
        return result
    
    def measure_batch_performance(self) -> List[Dict[str, Any]]:
        """Measure batch processing performance"""
        print("\n[2/4] Measuring batch performance...")
        
        queries = self.queries[self.config.warmup_queries:self.config.warmup_queries + 1000]
        results = []
        
        for batch_size in self.config.batch_sizes:
            print(f"  Testing batch size: {batch_size}")
            
            num_batches = len(queries) // batch_size
            latencies = []
            
            for i in range(num_batches):
                batch_queries = queries[i * batch_size:(i + 1) * batch_size]
                
                start = time.perf_counter()
                
                for query in batch_queries:
                    if HAS_PYVDB and self.db:
                        options = pyvdb.QueryOptions()
                        options.k = 10
                        self.db.search_vector(query.tolist(), options)
                    else:
                        time.sleep(0.0002)
                
                end = time.perf_counter()
                batch_latency = (end - start) * 1000.0
                latencies.append(batch_latency / batch_size)  # Per query latency
            
            result = {
                'batch_size': batch_size,
                'mean_latency_ms': float(np.mean(latencies)),
                'p95_latency_ms': float(np.percentile(latencies, 95)),
                'throughput_qps': batch_size / (np.mean(latencies) / 1000.0)
            }
            
            results.append(result)
            print(f"    Mean latency: {result['mean_latency_ms']:.3f}ms, Throughput: {result['throughput_qps']:.0f} QPS")
        
        return results
    
    def measure_concurrent_performance(self) -> List[Dict[str, Any]]:
        """Measure concurrent client performance"""
        print("\n[3/4] Measuring concurrent performance...")
        
        queries = self.queries[self.config.warmup_queries:self.config.warmup_queries + 1000]
        results = []
        
        for num_clients in self.config.concurrent_clients:
            print(f"  Testing {num_clients} concurrent clients...")
            
            queries_per_client = len(queries) // num_clients
            
            def client_worker(client_id: int):
                start_idx = client_id * queries_per_client
                end_idx = start_idx + queries_per_client
                client_queries = queries[start_idx:end_idx]
                
                latencies = []
                for query in client_queries:
                    start = time.perf_counter()
                    
                    if HAS_PYVDB and self.db:
                        options = pyvdb.QueryOptions()
                        options.k = 10
                        self.db.search_vector(query.tolist(), options)
                    else:
                        time.sleep(0.0002)
                    
                    end = time.perf_counter()
                    latencies.append((end - start) * 1000.0)
                
                return latencies
            
            all_latencies = []
            start_time = time.perf_counter()
            
            with ThreadPoolExecutor(max_workers=num_clients) as executor:
                futures = [executor.submit(client_worker, i) for i in range(num_clients)]
                for future in as_completed(futures):
                    all_latencies.extend(future.result())
            
            end_time = time.perf_counter()
            total_time = end_time - start_time
            
            result = {
                'concurrent_clients': num_clients,
                'total_queries': len(all_latencies),
                'mean_latency_ms': float(np.mean(all_latencies)),
                'p95_latency_ms': float(np.percentile(all_latencies, 95)),
                'p99_latency_ms': float(np.percentile(all_latencies, 99)),
                'throughput_qps': len(all_latencies) / total_time
            }
            
            results.append(result)
            print(f"    Mean latency: {result['mean_latency_ms']:.3f}ms, "
                  f"p99: {result['p99_latency_ms']:.3f}ms, "
                  f"Throughput: {result['throughput_qps']:.0f} QPS")
        
        return results
    
    def measure_sustained_load(self) -> List[Dict[str, Any]]:
        """Measure performance under sustained load"""
        print("\n[4/4] Measuring sustained load performance...")
        
        results = []
        
        for target_qps in self.config.target_qps:
            print(f"  Testing target QPS: {target_qps}")
            
            duration = min(self.config.duration_seconds, 30)  # Cap at 30s for testing
            target_interval = 1.0 / target_qps
            
            latencies = []
            start_time = time.time()
            query_idx = self.config.warmup_queries
            
            while time.time() - start_time < duration:
                query = self.queries[query_idx % len(self.queries)]
                query_idx += 1
                
                query_start = time.perf_counter()
                
                if HAS_PYVDB and self.db:
                    options = pyvdb.QueryOptions()
                    options.k = 10
                    self.db.search_vector(query.tolist(), options)
                else:
                    time.sleep(0.0002)
                
                query_end = time.perf_counter()
                latencies.append((query_end - query_start) * 1000.0)
                
                # Rate limiting
                elapsed = query_end - query_start
                sleep_time = max(0, target_interval - elapsed)
                time.sleep(sleep_time)
            
            actual_qps = len(latencies) / duration
            
            result = {
                'target_qps': target_qps,
                'actual_qps': float(actual_qps),
                'duration_sec': duration,
                'total_queries': len(latencies),
                'mean_latency_ms': float(np.mean(latencies)),
                'p95_latency_ms': float(np.percentile(latencies, 95)),
                'p99_latency_ms': float(np.percentile(latencies, 99))
            }
            
            results.append(result)
            print(f"    Actual QPS: {actual_qps:.0f}, Mean latency: {result['mean_latency_ms']:.3f}ms")
        
        return results
    
    def run_benchmark(self) -> Dict[str, Any]:
        """Run full benchmark suite"""
        self.setup()
        self.warmup()
        
        results = {
            'test_name': 'Latency & Throughput Benchmark',
            'version': '1.0.0',
            'timestamp': time.time(),
            'config': asdict(self.config),
            'results': {
                'latency_distribution': self.measure_latency_distribution(),
                'batch_performance': self.measure_batch_performance(),
                'concurrent_performance': self.measure_concurrent_performance(),
                'sustained_load': self.measure_sustained_load()
            }
        }
        
        # Cleanup
        if HAS_PYVDB and self.db:
            self.db.close()
        
        return results


def main():
    parser = argparse.ArgumentParser(description='HEKTOR Latency & Throughput Benchmark')
    parser.add_argument('--vectors', type=int, default=100000, help='Number of vectors')
    parser.add_argument('--dimension', type=int, default=512, help='Vector dimension')
    parser.add_argument('--queries', type=int, default=10000, help='Number of queries')
    parser.add_argument('--output', type=str, default='latency_throughput_results.json', 
                       help='Output file')
    
    args = parser.parse_args()
    
    config = LatencyBenchmarkConfig(
        vector_count=args.vectors,
        dimension=args.dimension,
        num_queries=args.queries,
        output_file=args.output
    )
    
    print("═══════════════════════════════════════════════════════")
    print("  HEKTOR Latency & Throughput Benchmark")
    print("═══════════════════════════════════════════════════════")
    print()
    
    benchmark = LatencyBenchmark(config)
    results = benchmark.run_benchmark()
    
    # Export results
    with open(config.output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print()
    print("═══════════════════════════════════════════════════════")
    print("  Benchmark Completed")
    print("═══════════════════════════════════════════════════════")
    print(f"Results saved to: {config.output_file}")
    print()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
