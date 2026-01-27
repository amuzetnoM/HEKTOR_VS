#!/usr/bin/env python3
"""
HEKTOR Benchmark Report Generator

Generates comprehensive HTML and JSON reports from benchmark results,
following audit standards from docs/.SCOPE/audit/

Audit Compliance: docs/.SCOPE/audit/quality/TEST_COVERAGE_REPORT.md

Version: 1.0.0
Date: 2026-01-24
"""

import argparse
import json
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any
import statistics


class BenchmarkReportGenerator:
    """Generate comprehensive benchmark reports"""
    
    def __init__(self, report_dir: Path, output_dir: Path):
        self.report_dir = report_dir
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.database_results = []
        self.studio_results = []
        self.system_info = {}
    
    def load_results(self):
        """Load all benchmark results"""
        print("Loading benchmark results...")
        
        # Load database results
        db_dir = self.report_dir / "database"
        if db_dir.exists():
            for result_file in db_dir.glob("*.json"):
                try:
                    with open(result_file) as f:
                        data = json.load(f)
                        self.database_results.append({
                            'file': result_file.name,
                            'data': data
                        })
                except Exception as e:
                    print(f"Warning: Failed to load {result_file}: {e}")
        
        # Load studio results
        studio_dir = self.report_dir / "studio"
        if studio_dir.exists():
            for result_file in studio_dir.glob("*.json"):
                try:
                    with open(result_file) as f:
                        data = json.load(f)
                        self.studio_results.append({
                            'file': result_file.name,
                            'data': data
                        })
                except Exception as e:
                    print(f"Warning: Failed to load {result_file}: {e}")
        
        # Load system info
        system_info_file = self.report_dir / "system_info.txt"
        if system_info_file.exists():
            with open(system_info_file) as f:
                self.system_info = {'raw': f.read()}
        
        print(f"Loaded {len(self.database_results)} database results")
        print(f"Loaded {len(self.studio_results)} studio results")
    
    def generate_summary(self) -> Dict[str, Any]:
        """Generate summary statistics"""
        summary = {
            'timestamp': datetime.now().isoformat(),
            'report_version': '1.0.0',
            'database': self._summarize_database_results(),
            'studio': self._summarize_studio_results(),
            'system_info': self.system_info
        }
        return summary
    
    def _summarize_database_results(self) -> Dict[str, Any]:
        """Summarize database benchmark results"""
        if not self.database_results:
            return {'status': 'no_results'}
        
        summary = {
            'total_tests': len(self.database_results),
            'tests': []
        }
        
        for result in self.database_results:
            test_summary = {
                'name': result.get('file', 'unknown'),
                'status': 'completed'
            }
            
            data = result.get('data', {})
            if 'results' in data:
                results = data['results']
                if 'latency_ms' in results:
                    test_summary['latency_ms'] = results['latency_ms']
                if 'throughput_ops_per_sec' in results:
                    test_summary['throughput'] = results['throughput_ops_per_sec']
            
            summary['tests'].append(test_summary)
        
        return summary
    
    def _summarize_studio_results(self) -> Dict[str, Any]:
        """Summarize studio benchmark results"""
        if not self.studio_results:
            return {'status': 'no_results'}
        
        summary = {
            'total_tests': len(self.studio_results),
            'tests': []
        }
        
        for result in self.studio_results:
            test_summary = {
                'name': result.get('file', 'unknown'),
                'status': 'completed'
            }
            
            data = result.get('data', {})
            if 'results' in data:
                test_summary['results'] = data['results']
            
            summary['tests'].append(test_summary)
        
        return summary
    
    def generate_html_report(self, summary: Dict[str, Any]):
        """Generate HTML report"""
        html_file = self.output_dir / "report.html"
        
        html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>HEKTOR Benchmark Report</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            line-height: 1.6;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background: #f5f5f5;
        }}
        .header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
            border-radius: 8px;
            margin-bottom: 30px;
        }}
        .header h1 {{
            margin: 0 0 10px 0;
            font-size: 2.5em;
        }}
        .header p {{
            margin: 5px 0;
            opacity: 0.9;
        }}
        .section {{
            background: white;
            padding: 30px;
            border-radius: 8px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .section h2 {{
            color: #667eea;
            margin-top: 0;
            border-bottom: 2px solid #667eea;
            padding-bottom: 10px;
        }}
        .metric {{
            display: inline-block;
            background: #f0f4ff;
            padding: 15px 20px;
            border-radius: 6px;
            margin: 10px 10px 10px 0;
            min-width: 200px;
        }}
        .metric-label {{
            font-size: 0.85em;
            color: #666;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }}
        .metric-value {{
            font-size: 1.8em;
            font-weight: bold;
            color: #667eea;
            margin-top: 5px;
        }}
        .status-pass {{
            color: #10b981;
            font-weight: bold;
        }}
        .status-fail {{
            color: #ef4444;
            font-weight: bold;
        }}
        .status-pending {{
            color: #f59e0b;
            font-weight: bold;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }}
        th, td {{
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #e5e7eb;
        }}
        th {{
            background: #f9fafb;
            font-weight: 600;
            color: #374151;
        }}
        tr:hover {{
            background: #f9fafb;
        }}
        .footer {{
            text-align: center;
            margin-top: 40px;
            padding: 20px;
            color: #666;
            font-size: 0.9em;
        }}
        code {{
            background: #f3f4f6;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
        }}
    </style>
</head>
<body>
    <div class="header">
        <h1>⬜️ HEKTOR Benchmark Report</h1>
        <p><strong>Version:</strong> {summary.get('report_version', '1.0.0')}</p>
        <p><strong>Generated:</strong> {summary.get('timestamp', 'N/A')}</p>
        <p><strong>Repository:</strong> amuzetnoM/hektor</p>
    </div>

    <div class="section">
        <h2>📊 Executive Summary</h2>
        <div class="metric">
            <div class="metric-label">Database Tests</div>
            <div class="metric-value">{summary.get('database', {}).get('total_tests', 0)}</div>
        </div>
        <div class="metric">
            <div class="metric-label">Studio Tests</div>
            <div class="metric-value">{summary.get('studio', {}).get('total_tests', 0)}</div>
        </div>
        <div class="metric">
            <div class="metric-label">Total Tests</div>
            <div class="metric-value">{summary.get('database', {}).get('total_tests', 0) + summary.get('studio', {}).get('total_tests', 0)}</div>
        </div>
    </div>

    <div class="section">
        <h2>🗄️ Database Benchmarks</h2>
        {self._generate_database_section_html(summary.get('database', {}))}
    </div>

    <div class="section">
        <h2>🖥️ Studio Benchmarks</h2>
        {self._generate_studio_section_html(summary.get('studio', {}))}
    </div>

    <div class="section">
        <h2>💻 System Information</h2>
        <pre style="background: #f3f4f6; padding: 15px; border-radius: 6px; overflow-x: auto;">
{summary.get('system_info', {}).get('raw', 'No system information available')}
        </pre>
    </div>

    <div class="footer">
        <p><strong>Audit Compliance:</strong> This report follows standards from <code>docs/.SCOPE/audit/</code></p>
        <p>Generated by HEKTOR Benchmark Suite v1.0.0</p>
    </div>
</body>
</html>
"""
        
        with open(html_file, 'w') as f:
            f.write(html_content)
        
        print(f"HTML report generated: {html_file}")
    
    def _generate_database_section_html(self, database_summary: Dict[str, Any]) -> str:
        """Generate HTML for database section"""
        if database_summary.get('status') == 'no_results':
            return '<p class="status-pending">No database results available</p>'
        
        html = '<table><thead><tr><th>Test</th><th>Status</th><th>Metrics</th></tr></thead><tbody>'
        
        for test in database_summary.get('tests', []):
            name = test.get('name', 'Unknown')
            status = test.get('status', 'unknown')
            
            metrics_html = ''
            if 'latency_ms' in test:
                latency = test['latency_ms']
                metrics_html += f"<div>p50: {latency.get('p50', 'N/A')}ms, p99: {latency.get('p99', 'N/A')}ms</div>"
            if 'throughput' in test:
                metrics_html += f"<div>Throughput: {test['throughput']:.0f} ops/sec</div>"
            
            html += f'<tr><td>{name}</td><td class="status-pass">{status}</td><td>{metrics_html}</td></tr>'
        
        html += '</tbody></table>'
        return html
    
    def _generate_studio_section_html(self, studio_summary: Dict[str, Any]) -> str:
        """Generate HTML for studio section"""
        if studio_summary.get('status') == 'no_results':
            return '<p class="status-pending">No studio results available</p>'
        
        html = '<table><thead><tr><th>Test</th><th>Status</th><th>Details</th></tr></thead><tbody>'
        
        for test in studio_summary.get('tests', []):
            name = test.get('name', 'Unknown')
            status = test.get('status', 'unknown')
            
            html += f'<tr><td>{name}</td><td class="status-pass">{status}</td><td>See detailed results</td></tr>'
        
        html += '</tbody></table>'
        return html
    
    def generate_json_report(self, summary: Dict[str, Any]):
        """Generate JSON report"""
        json_file = self.output_dir / "report.json"
        
        with open(json_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        print(f"JSON report generated: {json_file}")
    
    def generate_reports(self, formats: List[str]):
        """Generate reports in specified formats"""
        summary = self.generate_summary()
        
        if 'html' in formats:
            self.generate_html_report(summary)
        
        if 'json' in formats:
            self.generate_json_report(summary)


def main():
    parser = argparse.ArgumentParser(description='HEKTOR Benchmark Report Generator')
    parser.add_argument('--report-dir', type=str, required=True,
                       help='Directory containing benchmark results')
    parser.add_argument('--output', type=str, required=True,
                       help='Output directory for reports')
    parser.add_argument('--format', type=str, default='html,json',
                       help='Output formats (comma-separated): html, json')
    
    args = parser.parse_args()
    
    report_dir = Path(args.report_dir)
    output_dir = Path(args.output)
    formats = [f.strip() for f in args.format.split(',')]
    
    if not report_dir.exists():
        print(f"Error: Report directory not found: {report_dir}")
        return 1
    
    print("═══════════════════════════════════════════════════════")
    print("  HEKTOR Benchmark Report Generator")
    print("═══════════════════════════════════════════════════════")
    print()
    
    generator = BenchmarkReportGenerator(report_dir, output_dir)
    generator.load_results()
    generator.generate_reports(formats)
    
    print()
    print("═══════════════════════════════════════════════════════")
    print("  Report Generation Completed")
    print("═══════════════════════════════════════════════════════")
    print()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
