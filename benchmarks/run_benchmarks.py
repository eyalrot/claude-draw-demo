#!/usr/bin/env python3
"""Main benchmark runner for Claude Draw performance testing."""

import sys
import json
from datetime import datetime
from pathlib import Path

from benchmark_shape_creation import run_shape_creation_benchmarks
from benchmark_containers import run_container_benchmarks
from benchmark_utils import BenchmarkSuite


def save_results_to_json(suite: BenchmarkSuite, filename: str):
    """Save benchmark results to JSON file."""
    results_data = {
        'timestamp': datetime.now().isoformat(),
        'summary': suite.get_summary(),
        'individual_results': [result.to_dict() for result in suite.results]
    }
    
    with open(filename, 'w') as f:
        json.dump(results_data, f, indent=2)
    
    print(f"📁 Results saved to: {filename}")


def main():
    """Run all benchmarks and generate reports."""
    print("🎯 Claude Draw Performance Benchmarks")
    print("="*80)
    print(f"📅 Started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    
    # Create results directory
    results_dir = Path("benchmark_results")
    results_dir.mkdir(exist_ok=True)
    
    # Combined suite for all results
    combined_suite = BenchmarkSuite()
    
    try:
        # Run shape creation benchmarks
        print("🔥 Phase 1: Shape Creation Performance")
        shape_suite = run_shape_creation_benchmarks()
        combined_suite.results.extend(shape_suite.results)
        
        print("\n" + "="*60)
        
        # Run container benchmarks
        print("🔥 Phase 2: Container Performance")
        container_suite = run_container_benchmarks()
        combined_suite.results.extend(container_suite.results)
        
        # Print combined summary
        combined_suite.print_summary()
        
        # Save results
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Save individual suite results
        save_results_to_json(shape_suite, f"benchmark_results/shape_creation_{timestamp}.json")
        save_results_to_json(container_suite, f"benchmark_results/containers_{timestamp}.json")
        
        # Save combined results
        save_results_to_json(combined_suite, f"benchmark_results/combined_{timestamp}.json")
        
        print(f"\n✅ All benchmarks completed successfully!")
        
        # Print key findings
        print("\n🎯 KEY FINDINGS:")
        summary = combined_suite.get_summary()
        if summary:
            print(f"   • Total objects created: {summary['total_objects_created']:,}")
            print(f"   • Average creation time: {summary['time_per_object_stats']['mean']:.2f} μs per object")
            print(f"   • Average throughput: {summary['objects_per_second_stats']['mean']:,.0f} objects/second")
            print(f"   • Average memory delta: {summary['memory_delta_stats']['mean']:+.2f} MB")
        
        return 0
        
    except Exception as e:
        print(f"❌ Benchmark failed: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())