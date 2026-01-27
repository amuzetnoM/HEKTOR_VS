#!/usr/bin/env python3
"""
HEKTOR Perceptual Quantization (PQ/HLG) HDR Benchmark

Comprehensive benchmark for SMPTE ST 2084 (PQ) and HLG transfer curves.
Includes visual verification with screenshots at each checkpoint.

This is HEKTOR's 2084 implementation - industry-first HDR perceptual
quantization for vector databases.

Audit Compliance: docs/.SCOPE/audit/quality/TEST_COVERAGE_REPORT.md

Version: 1.0.0
Date: 2026-01-27
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
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    from matplotlib.patches import Rectangle
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("Warning: matplotlib not available, visual output disabled")


@dataclass
class PQBenchmarkConfig:
    """PQ/HLG benchmark configuration"""
    test_scales: List[int] = None
    num_samples: int = 10000
    luminance_range: Tuple[float, float] = (0.0, 10000.0)  # nits
    checkpoint_interval: int = 1000
    enable_visualization: bool = True
    output_dir: str = "pq_hlg_results"
    output_file: str = "pq_hlg_benchmark.json"
    
    def __post_init__(self):
        if self.test_scales is None:
            self.test_scales = [100, 1000, 10000, 100000]  # SDR to HDR10+ range


class PQCurve:
    """SMPTE ST 2084 (PQ) Transfer Function Implementation"""
    
    # ST 2084 constants
    M1 = 2610.0 / 16384.0  # 0.1593017578125
    M2 = 2523.0 / 4096.0 * 128.0  # 78.84375
    C1 = 3424.0 / 4096.0  # 0.8359375
    C2 = 2413.0 / 4096.0 * 32.0  # 18.8515625
    C3 = 2392.0 / 4096.0 * 32.0  # 18.6875
    
    @staticmethod
    def encode(luminance: float) -> float:
        """Encode linear luminance (nits) to PQ signal [0, 1]"""
        # Normalize to 0-1 range (10,000 nits max)
        Y = luminance / 10000.0
        Y = max(0.0, min(1.0, Y))
        
        if Y == 0.0:
            return 0.0
        
        Y_pow = np.power(Y, PQCurve.M1)
        numerator = PQCurve.C1 + PQCurve.C2 * Y_pow
        denominator = 1.0 + PQCurve.C3 * Y_pow
        
        V = np.power(numerator / denominator, PQCurve.M2)
        return float(V)
    
    @staticmethod
    def decode(signal: float) -> float:
        """Decode PQ signal [0, 1] to linear luminance (nits)"""
        V = max(0.0, min(1.0, signal))
        
        if V == 0.0:
            return 0.0
        
        V_pow = np.power(V, 1.0 / PQCurve.M2)
        numerator = max(V_pow - PQCurve.C1, 0.0)
        denominator = PQCurve.C2 - PQCurve.C3 * V_pow
        
        if denominator == 0.0:
            return 0.0
        
        Y = np.power(numerator / denominator, 1.0 / PQCurve.M1)
        return float(Y * 10000.0)
    
    @staticmethod
    def encode_batch(luminances: np.ndarray) -> np.ndarray:
        """Batch encode luminances"""
        return np.array([PQCurve.encode(l) for l in luminances])
    
    @staticmethod
    def decode_batch(signals: np.ndarray) -> np.ndarray:
        """Batch decode signals"""
        return np.array([PQCurve.decode(s) for s in signals])


class HLGCurve:
    """HLG (Rec. 2100) Transfer Function Implementation"""
    
    A = 0.17883277
    B = 0.28466892
    C = 0.55991073
    
    @staticmethod
    def encode(linear: float) -> float:
        """Encode linear scene light to HLG signal [0, 1]"""
        E = max(0.0, min(1.0, linear))
        
        if E <= 1.0 / 12.0:
            return float(np.sqrt(3.0 * E))
        else:
            return float(HLGCurve.A * np.log(12.0 * E - HLGCurve.B) + HLGCurve.C)
    
    @staticmethod
    def decode(signal: float) -> float:
        """Decode HLG signal [0, 1] to linear scene light"""
        E_prime = max(0.0, min(1.0, signal))
        
        if E_prime <= 0.5:
            return float(E_prime * E_prime / 3.0)
        else:
            return float((np.exp((E_prime - HLGCurve.C) / HLGCurve.A) + HLGCurve.B) / 12.0)
    
    @staticmethod
    def encode_batch(linear: np.ndarray) -> np.ndarray:
        """Batch encode"""
        return np.array([HLGCurve.encode(l) for l in linear])
    
    @staticmethod
    def decode_batch(signals: np.ndarray) -> np.ndarray:
        """Batch decode"""
        return np.array([HLGCurve.decode(s) for s in signals])


class PQHLGBenchmark:
    """PQ/HLG HDR Benchmark Suite"""
    
    def __init__(self, config: PQBenchmarkConfig):
        self.config = config
        self.output_dir = Path(config.output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.results = {
            'pq_tests': [],
            'hlg_tests': [],
            'visual_checkpoints': []
        }
    
    def benchmark_pq_encoding_accuracy(self) -> Dict[str, Any]:
        """Test PQ encoding/decoding accuracy"""
        print("\n[1/6] PQ Encoding Accuracy Test")
        
        # Test standard HDR reference points
        test_points = {
            '1 nit (dark)': 1.0,
            '10 nits (dim)': 10.0,
            '100 nits (SDR peak)': 100.0,
            '200 nits (bright SDR)': 200.0,
            '400 nits (HDR start)': 400.0,
            '1000 nits (HDR10)': 1000.0,
            '4000 nits (HDR10+ peak)': 4000.0,
            '10000 nits (max)': 10000.0
        }
        
        results = {}
        max_error = 0.0
        
        for name, luminance in test_points.items():
            encoded = PQCurve.encode(luminance)
            decoded = PQCurve.decode(encoded)
            error_pct = abs(decoded - luminance) / luminance * 100.0 if luminance > 0 else 0
            max_error = max(max_error, error_pct)
            
            results[name] = {
                'input_nits': luminance,
                'encoded_signal': float(encoded),
                'decoded_nits': float(decoded),
                'error_percent': float(error_pct)
            }
            
            print(f"  {name:25s}: {encoded:.4f} -> {decoded:.2f} nits (error: {error_pct:.3f}%)")
        
        results['max_error_percent'] = float(max_error)
        results['status'] = 'PASS' if max_error < 1.0 else 'FAIL'
        
        return results
    
    def benchmark_pq_performance(self) -> Dict[str, Any]:
        """Test PQ encoding/decoding performance"""
        print("\n[2/6] PQ Performance Benchmark")
        
        results = {}
        
        for scale in self.config.test_scales:
            print(f"  Testing {scale:,} samples...")
            
            # Generate test data
            luminances = np.linspace(0.0, 10000.0, scale)
            
            # Encode performance
            start = time.perf_counter()
            encoded = PQCurve.encode_batch(luminances)
            encode_time = time.perf_counter() - start
            
            # Decode performance
            start = time.perf_counter()
            decoded = PQCurve.decode_batch(encoded)
            decode_time = time.perf_counter() - start
            
            results[f'{scale}_samples'] = {
                'encode_time_ms': float(encode_time * 1000),
                'decode_time_ms': float(decode_time * 1000),
                'encode_throughput_samples_per_sec': float(scale / encode_time),
                'decode_throughput_samples_per_sec': float(scale / decode_time)
            }
            
            print(f"    Encode: {encode_time*1000:.2f}ms ({scale/encode_time:,.0f} samples/s)")
            print(f"    Decode: {decode_time*1000:.2f}ms ({scale/decode_time:,.0f} samples/s)")
        
        return results
    
    def benchmark_hlg_accuracy(self) -> Dict[str, Any]:
        """Test HLG encoding/decoding accuracy"""
        print("\n[3/6] HLG Encoding Accuracy Test")
        
        test_points = {
            'Black (0.0)': 0.0,
            'Transition (1/12)': 1.0/12.0,
            'Mid gray (0.18)': 0.18,
            'Reference (0.5)': 0.5,
            'Peak white (1.0)': 1.0
        }
        
        results = {}
        max_error = 0.0
        
        for name, linear in test_points.items():
            encoded = HLGCurve.encode(linear)
            decoded = HLGCurve.decode(encoded)
            error_pct = abs(decoded - linear) / max(linear, 0.001) * 100.0
            max_error = max(max_error, error_pct)
            
            results[name] = {
                'input': linear,
                'encoded': float(encoded),
                'decoded': float(decoded),
                'error_percent': float(error_pct)
            }
            
            print(f"  {name:25s}: {encoded:.4f} -> {decoded:.4f} (error: {error_pct:.3f}%)")
        
        results['max_error_percent'] = float(max_error)
        results['status'] = 'PASS' if max_error < 0.1 else 'FAIL'
        
        return results
    
    def generate_pq_curve_visualization(self):
        """Generate PQ curve visualization with checkpoints"""
        if not HAS_MATPLOTLIB:
            print("  Skipping visualization (matplotlib not available)")
            return None
        
        print("\n[4/6] Generating PQ Curve Visualization")
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('HEKTOR PQ (SMPTE ST 2084) Transfer Curve Analysis', fontsize=16, fontweight='bold')
        
        # 1. Encoding curve
        ax = axes[0, 0]
        luminances = np.linspace(0, 10000, 1000)
        encoded = PQCurve.encode_batch(luminances)
        
        ax.plot(luminances, encoded, 'b-', linewidth=2, label='PQ Encoding')
        ax.axvline(100, color='g', linestyle='--', alpha=0.5, label='SDR Peak (100 nits)')
        ax.axvline(1000, color='orange', linestyle='--', alpha=0.5, label='HDR10 (1000 nits)')
        ax.axvline(4000, color='r', linestyle='--', alpha=0.5, label='HDR10+ (4000 nits)')
        
        ax.set_xlabel('Luminance (nits)', fontsize=12)
        ax.set_ylabel('PQ Signal [0, 1]', fontsize=12)
        ax.set_title('PQ Encoding Curve', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        ax.set_xlim([0, 10000])
        ax.set_ylim([0, 1])
        
        # 2. Log-scale view
        ax = axes[0, 1]
        ax.semilogx(luminances[1:], encoded[1:], 'b-', linewidth=2)
        ax.axvline(100, color='g', linestyle='--', alpha=0.5)
        ax.axvline(1000, color='orange', linestyle='--', alpha=0.5)
        ax.axvline(4000, color='r', linestyle='--', alpha=0.5)
        
        ax.set_xlabel('Luminance (nits) - Log Scale', fontsize=12)
        ax.set_ylabel('PQ Signal [0, 1]', fontsize=12)
        ax.set_title('PQ Curve (Log Scale)', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.set_xlim([0.1, 10000])
        ax.set_ylim([0, 1])
        
        # 3. Decoding curve
        ax = axes[1, 0]
        signals = np.linspace(0, 1, 1000)
        decoded = PQCurve.decode_batch(signals)
        
        ax.plot(signals, decoded, 'r-', linewidth=2, label='PQ Decoding')
        ax.axhline(100, color='g', linestyle='--', alpha=0.5)
        ax.axhline(1000, color='orange', linestyle='--', alpha=0.5)
        ax.axhline(4000, color='r', linestyle='--', alpha=0.5)
        
        ax.set_xlabel('PQ Signal [0, 1]', fontsize=12)
        ax.set_ylabel('Luminance (nits)', fontsize=12)
        ax.set_title('PQ Decoding Curve', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        
        # 4. Round-trip error
        ax = axes[1, 1]
        roundtrip_decoded = PQCurve.decode_batch(PQCurve.encode_batch(luminances))
        errors = np.abs(roundtrip_decoded - luminances) / np.maximum(luminances, 0.001) * 100
        
        ax.semilogy(luminances, errors, 'purple', linewidth=2)
        ax.axhline(1.0, color='orange', linestyle='--', alpha=0.5, label='1% Error')
        ax.axhline(0.1, color='g', linestyle='--', alpha=0.5, label='0.1% Error')
        
        ax.set_xlabel('Luminance (nits)', fontsize=12)
        ax.set_ylabel('Round-trip Error (%)', fontsize=12)
        ax.set_title('PQ Encoding/Decoding Error', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        ax.set_xlim([0, 10000])
        
        plt.tight_layout()
        
        # Save figure
        output_path = self.output_dir / 'pq_curve_analysis.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close()
        
        print(f"  ✓ Saved: {output_path}")
        
        return str(output_path)
    
    def generate_hlg_curve_visualization(self):
        """Generate HLG curve visualization"""
        if not HAS_MATPLOTLIB:
            print("  Skipping visualization (matplotlib not available)")
            return None
        
        print("\n[5/6] Generating HLG Curve Visualization")
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('HEKTOR HLG (Rec. 2100) Transfer Curve Analysis', fontsize=16, fontweight='bold')
        
        # 1. Encoding curve
        ax = axes[0, 0]
        linear = np.linspace(0, 1, 1000)
        encoded = HLGCurve.encode_batch(linear)
        
        ax.plot(linear, encoded, 'b-', linewidth=2, label='HLG Encoding')
        ax.axvline(1.0/12.0, color='r', linestyle='--', alpha=0.5, label='Transition (1/12)')
        ax.axvline(0.5, color='orange', linestyle='--', alpha=0.5, label='Reference')
        
        ax.set_xlabel('Linear Scene Light [0, 1]', fontsize=12)
        ax.set_ylabel('HLG Signal [0, 1]', fontsize=12)
        ax.set_title('HLG Encoding Curve', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        
        # 2. Piecewise regions
        ax = axes[0, 1]
        # Linear region
        linear_region = linear[linear <= 1.0/12.0]
        encoded_linear = HLGCurve.encode_batch(linear_region)
        ax.plot(linear_region, encoded_linear, 'g-', linewidth=3, label='Linear Region (E ≤ 1/12)')
        
        # Logarithmic region
        log_region = linear[linear > 1.0/12.0]
        encoded_log = HLGCurve.encode_batch(log_region)
        ax.plot(log_region, encoded_log, 'b-', linewidth=3, label='Logarithmic Region (E > 1/12)')
        
        ax.axvline(1.0/12.0, color='r', linestyle='--', alpha=0.5, linewidth=2)
        
        ax.set_xlabel('Linear Scene Light [0, 1]', fontsize=12)
        ax.set_ylabel('HLG Signal [0, 1]', fontsize=12)
        ax.set_title('HLG Piecewise Function', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        
        # 3. Decoding curve
        ax = axes[1, 0]
        signals = np.linspace(0, 1, 1000)
        decoded = HLGCurve.decode_batch(signals)
        
        ax.plot(signals, decoded, 'r-', linewidth=2, label='HLG Decoding')
        ax.axvline(0.5, color='orange', linestyle='--', alpha=0.5)
        
        ax.set_xlabel('HLG Signal [0, 1]', fontsize=12)
        ax.set_ylabel('Linear Scene Light [0, 1]', fontsize=12)
        ax.set_title('HLG Decoding Curve', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        
        # 4. Round-trip error
        ax = axes[1, 1]
        roundtrip_decoded = HLGCurve.decode_batch(HLGCurve.encode_batch(linear))
        errors = np.abs(roundtrip_decoded - linear) / np.maximum(linear, 0.001) * 100
        
        ax.plot(linear, errors, 'purple', linewidth=2)
        ax.axhline(0.1, color='g', linestyle='--', alpha=0.5, label='0.1% Error')
        ax.axhline(0.01, color='b', linestyle='--', alpha=0.5, label='0.01% Error')
        
        ax.set_xlabel('Linear Scene Light [0, 1]', fontsize=12)
        ax.set_ylabel('Round-trip Error (%)', fontsize=12)
        ax.set_title('HLG Encoding/Decoding Error', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        ax.set_yscale('log')
        
        plt.tight_layout()
        
        # Save figure
        output_path = self.output_dir / 'hlg_curve_analysis.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close()
        
        print(f"  ✓ Saved: {output_path}")
        
        return str(output_path)
    
    def generate_comparison_visualization(self):
        """Generate PQ vs HLG comparison"""
        if not HAS_MATPLOTLIB:
            return None
        
        print("\n[6/6] Generating PQ vs HLG Comparison")
        
        fig, axes = plt.subplots(1, 2, figsize=(16, 6))
        fig.suptitle('PQ vs HLG: HDR Transfer Function Comparison', fontsize=16, fontweight='bold')
        
        # Normalize for comparison
        luminances = np.linspace(0, 10000, 1000)
        pq_encoded = PQCurve.encode_batch(luminances)
        
        # HLG operates on normalized scene light
        hlg_linear = luminances / 1000.0  # Normalize to ~1000 nits peak
        hlg_linear = np.clip(hlg_linear, 0, 1)
        hlg_encoded = HLGCurve.encode_batch(hlg_linear)
        
        # 1. Overlay comparison
        ax = axes[0]
        ax.plot(luminances, pq_encoded, 'b-', linewidth=2, label='PQ (ST 2084)')
        ax.plot(luminances, hlg_encoded, 'r-', linewidth=2, label='HLG (Rec. 2100)')
        ax.axvline(1000, color='g', linestyle='--', alpha=0.3, label='1000 nits reference')
        
        ax.set_xlabel('Luminance (nits)', fontsize=12)
        ax.set_ylabel('Encoded Signal [0, 1]', fontsize=12)
        ax.set_title('Transfer Curve Overlay', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        ax.set_xlim([0, 10000])
        
        # 2. Difference plot
        ax = axes[1]
        difference = pq_encoded - hlg_encoded
        ax.plot(luminances, difference, 'purple', linewidth=2)
        ax.axhline(0, color='black', linestyle='-', alpha=0.3)
        ax.fill_between(luminances, difference, 0, alpha=0.3, color='purple')
        
        ax.set_xlabel('Luminance (nits)', fontsize=12)
        ax.set_ylabel('PQ - HLG Difference', fontsize=12)
        ax.set_title('Transfer Function Difference', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.set_xlim([0, 10000])
        
        plt.tight_layout()
        
        output_path = self.output_dir / 'pq_vs_hlg_comparison.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close()
        
        print(f"  ✓ Saved: {output_path}")
        
        return str(output_path)
    
    def run(self) -> Dict[str, Any]:
        """Run complete PQ/HLG benchmark"""
        print("\n" + "="*80)
        print("HEKTOR PQ/HLG HDR BENCHMARK")
        print("SMPTE ST 2084 & Rec. 2100 Implementation")
        print("="*80)
        
        results = {
            'test_name': 'PQ/HLG HDR Benchmark',
            'version': '1.0.0',
            'timestamp': time.time(),
            'config': asdict(self.config)
        }
        
        # Run tests
        results['pq_accuracy'] = self.benchmark_pq_encoding_accuracy()
        results['pq_performance'] = self.benchmark_pq_performance()
        results['hlg_accuracy'] = self.benchmark_hlg_accuracy()
        
        # Generate visualizations
        if self.config.enable_visualization and HAS_MATPLOTLIB:
            visualizations = []
            
            pq_viz = self.generate_pq_curve_visualization()
            if pq_viz:
                visualizations.append(pq_viz)
            
            hlg_viz = self.generate_hlg_curve_visualization()
            if hlg_viz:
                visualizations.append(hlg_viz)
            
            comp_viz = self.generate_comparison_visualization()
            if comp_viz:
                visualizations.append(comp_viz)
            
            results['visualizations'] = visualizations
        
        # Export results
        output_file = self.output_dir / self.config.output_file
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n✓ Results exported to: {output_file}")
        
        # Summary
        print("\n" + "="*80)
        print("BENCHMARK COMPLETE")
        print("="*80)
        print(f"\nPQ Accuracy: {results['pq_accuracy']['status']}")
        print(f"HLG Accuracy: {results['hlg_accuracy']['status']}")
        
        if 'visualizations' in results:
            print(f"\nVisualizations: {len(results['visualizations'])} generated")
            for viz in results['visualizations']:
                print(f"  - {Path(viz).name}")
        
        return results


def main():
    parser = argparse.ArgumentParser(description='HEKTOR PQ/HLG HDR Benchmark')
    parser.add_argument('--samples', type=int, default=10000,
                       help='Number of samples for performance tests')
    parser.add_argument('--no-viz', action='store_true',
                       help='Disable visualization generation')
    parser.add_argument('--output-dir', type=str, default='pq_hlg_results',
                       help='Output directory')
    
    args = parser.parse_args()
    
    # Install matplotlib if needed
    if not HAS_MATPLOTLIB and not args.no_viz:
        print("\nAttempting to install matplotlib...")
        import subprocess
        try:
            subprocess.check_call([sys.executable, '-m', 'pip', 'install', '--user', 'matplotlib'])
            print("✓ matplotlib installed successfully")
            print("Please re-run the benchmark to generate visualizations")
        except:
            print("⚠ Could not install matplotlib automatically")
            print("Run: pip install matplotlib")
    
    config = PQBenchmarkConfig(
        num_samples=args.samples,
        enable_visualization=not args.no_viz,
        output_dir=args.output_dir
    )
    
    benchmark = PQHLGBenchmark(config)
    results = benchmark.run()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
