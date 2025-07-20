# Claude Draw Performance Benchmarks

This directory contains performance benchmarks for the Claude Draw library, focusing on shape creation and container performance.

## Overview

The benchmark suite measures:
- **Shape Creation Performance**: Time to create 1000 and 5000 mixed shapes
- **Container Operations**: Adding shapes to Groups and Layers
- **Hierarchical Structures**: Complex nested container performance
- **Memory Usage**: Memory consumption during operations

## Quick Start

```bash
# Install dependencies
pip install psutil

# Run all benchmarks
cd benchmarks
python run_benchmarks.py
```

## Results Summary

Based on the latest benchmark run:

### Shape Creation Performance
- **1000 Mixed Shapes**: ~22-16 μs per object (43K-60K objects/sec)
- **5000 Mixed Shapes**: ~24-19 μs per object (42K-53K objects/sec)
- **Best Method**: Random properties > Factory functions > Direct instantiation

### Container Performance  
- **Group Operations**: ~21 μs per shape (47K shapes/sec)
- **Layer Operations**: ~21 μs per shape (46K shapes/sec)
- **Hierarchical Structures**: ~22 μs per shape (46K shapes/sec)

### Memory Efficiency
- **Shape Creation**: ~1-3 MB for 1000-5000 shapes
- **Container Operations**: Minimal memory overhead
- **Bounding Box Calculations**: ~7 μs per calculation

## Benchmark Files

- `benchmark_utils.py`: Core utilities and measurement framework
- `benchmark_shape_creation.py`: Shape creation performance tests
- `benchmark_containers.py`: Container operation tests  
- `run_benchmarks.py`: Main runner that executes all benchmarks

## Key Findings

1. **Factory Functions**: Slightly faster than direct instantiation
2. **Random Properties**: Most efficient due to pre-generated colors
3. **Container Overhead**: Minimal performance impact
4. **Memory Usage**: Very efficient, <1KB per object average
5. **Scaling**: Linear performance scaling from 1K to 5K objects

## Understanding Results

- **Execution Time**: Total time to complete the benchmark
- **Objects/Second**: Throughput rate  
- **Time per Object**: Average creation time in microseconds
- **Memory Delta**: Memory consumption change in MB
- **Peak Memory**: Maximum memory usage during benchmark

## Future Improvements

- Add rendering performance benchmarks
- Test with complex transformations
- Compare with other graphics libraries
- Profile specific bottlenecks
- Add stress tests for larger datasets

## Dependencies

- `psutil`: For memory monitoring
- `claude_draw`: The library being benchmarked