#!/usr/bin/env python3
"""Comprehensive benchmarks comparing C++ and Python performance"""

import sys
import os
import time
import numpy as np
import math
from dataclasses import dataclass
from typing import List, Tuple
import matplotlib.pyplot as plt

# Add build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../build-release/src/bindings'))

try:
    import _claude_draw_cpp as cpp
except ImportError:
    print("Failed to import C++ bindings. Make sure to build first.")
    sys.exit(1)

# Pure Python implementations for comparison
@dataclass
class PyPoint2D:
    x: float
    y: float
    
    def __add__(self, other):
        return PyPoint2D(self.x + other.x, self.y + other.y)
    
    def distance_to(self, other):
        dx = other.x - self.x
        dy = other.y - self.y
        return math.sqrt(dx * dx + dy * dy)
    
    def normalized(self):
        mag = math.sqrt(self.x * self.x + self.y * self.y)
        if mag == 0:
            return PyPoint2D(0, 0)
        return PyPoint2D(self.x / mag, self.y / mag)

@dataclass
class PyColor:
    r: int
    g: int
    b: int
    a: int
    
    def blend_over(self, other):
        # Simplified alpha blending
        alpha = self.a / 255.0
        inv_alpha = 1.0 - alpha
        
        return PyColor(
            int(self.r * alpha + other.r * inv_alpha),
            int(self.g * alpha + other.g * inv_alpha),
            int(self.b * alpha + other.b * inv_alpha),
            255
        )

class PyTransform2D:
    def __init__(self, matrix=None):
        self.matrix = matrix if matrix else [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    
    def transform_point(self, p):
        x = self.matrix[0][0] * p.x + self.matrix[0][1] * p.y + self.matrix[0][2]
        y = self.matrix[1][0] * p.x + self.matrix[1][1] * p.y + self.matrix[1][2]
        return PyPoint2D(x, y)
    
    @staticmethod
    def rotate(angle):
        c = math.cos(angle)
        s = math.sin(angle)
        return PyTransform2D([[c, -s, 0], [s, c, 0], [0, 0, 1]])

# Benchmark utilities
def benchmark(func, iterations=1000, warmup=100):
    """Run a function multiple times and measure average time"""
    # Warmup
    for _ in range(warmup):
        func()
    
    # Actual benchmark
    start = time.perf_counter()
    for _ in range(iterations):
        func()
    end = time.perf_counter()
    
    return (end - start) / iterations * 1000  # Return time in milliseconds

def format_speedup(cpp_time, py_time):
    """Format speedup factor"""
    speedup = py_time / cpp_time
    return f"{speedup:.1f}x faster" if speedup > 1 else f"{1/speedup:.1f}x slower"

# Benchmark functions
def bench_point_construction():
    """Benchmark Point2D construction"""
    print("\n=== Point2D Construction ===")
    
    # C++
    cpp_time = benchmark(lambda: cpp.Point2D(1.5, 2.5), 10000)
    
    # Python
    py_time = benchmark(lambda: PyPoint2D(1.5, 2.5), 10000)
    
    print(f"C++: {cpp_time:.3f} ms")
    print(f"Python: {py_time:.3f} ms")
    print(f"C++ is {format_speedup(cpp_time, py_time)}")
    
    return cpp_time, py_time

def bench_point_distance():
    """Benchmark point distance calculation"""
    print("\n=== Point2D Distance ===")
    
    # Setup
    cpp_p1 = cpp.Point2D(0, 0)
    cpp_p2 = cpp.Point2D(3, 4)
    py_p1 = PyPoint2D(0, 0)
    py_p2 = PyPoint2D(3, 4)
    
    # C++
    cpp_time = benchmark(lambda: cpp_p1.distance_to(cpp_p2), 10000)
    
    # Python
    py_time = benchmark(lambda: py_p1.distance_to(py_p2), 10000)
    
    print(f"C++: {cpp_time:.3f} ms")
    print(f"Python: {py_time:.3f} ms")
    print(f"C++ is {format_speedup(cpp_time, py_time)}")
    
    return cpp_time, py_time

def bench_color_blending():
    """Benchmark color blending"""
    print("\n=== Color Blending ===")
    
    # Setup
    cpp_fg = cpp.Color(255, 0, 0, 128)
    cpp_bg = cpp.Color(0, 0, 255, 255)
    py_fg = PyColor(255, 0, 0, 128)
    py_bg = PyColor(0, 0, 255, 255)
    
    # C++
    cpp_time = benchmark(lambda: cpp_fg.blend_over(cpp_bg), 10000)
    
    # Python
    py_time = benchmark(lambda: py_fg.blend_over(py_bg), 10000)
    
    print(f"C++: {cpp_time:.3f} ms")
    print(f"Python: {py_time:.3f} ms")
    print(f"C++ is {format_speedup(cpp_time, py_time)}")
    
    return cpp_time, py_time

def bench_transform_point():
    """Benchmark point transformation"""
    print("\n=== Transform Point ===")
    
    # Setup
    cpp_t = cpp.Transform2D.rotate(0.5)
    cpp_p = cpp.Point2D(10, 20)
    py_t = PyTransform2D.rotate(0.5)
    py_p = PyPoint2D(10, 20)
    
    # C++
    cpp_time = benchmark(lambda: cpp_t.transform_point(cpp_p), 10000)
    
    # Python
    py_time = benchmark(lambda: py_t.transform_point(py_p), 10000)
    
    print(f"C++: {cpp_time:.3f} ms")
    print(f"Python: {py_time:.3f} ms")
    print(f"C++ is {format_speedup(cpp_time, py_time)}")
    
    return cpp_time, py_time

def bench_batch_operations():
    """Benchmark batch operations"""
    print("\n=== Batch Operations (10,000 points) ===")
    
    count = 10000
    
    # Batch point creation
    print("\nBatch Point Creation:")
    cpp_time = benchmark(lambda: cpp.batch.create_points(count, 0, 0), 100)
    py_time = benchmark(lambda: [PyPoint2D(0, 0) for _ in range(count)], 100)
    
    print(f"C++: {cpp_time:.3f} ms")
    print(f"Python: {py_time:.3f} ms")
    print(f"C++ is {format_speedup(cpp_time, py_time)}")
    
    # Batch transformation
    print("\nBatch Transform:")
    points = np.random.randn(count, 2).astype(np.float32)
    transform = cpp.Transform2D.scale(2, 2)
    
    def cpp_batch_transform():
        pts = points.copy()
        cpp.batch.transform_points_batch(transform, pts)
    
    def py_batch_transform():
        pts = points.copy()
        for i in range(len(pts)):
            pts[i] *= 2  # Simplified transform
    
    cpp_time = benchmark(cpp_batch_transform, 100)
    py_time = benchmark(py_batch_transform, 100)
    
    print(f"C++: {cpp_time:.3f} ms")
    print(f"Python: {py_time:.3f} ms")
    print(f"C++ is {format_speedup(cpp_time, py_time)}")
    
    return cpp_time, py_time

def bench_memory_allocation():
    """Benchmark memory allocation strategies"""
    print("\n=== Memory Allocation (1000 objects) ===")
    
    count = 1000
    
    # Standard allocation
    print("\nStandard Allocation:")
    cpp_time = benchmark(lambda: [cpp.Point2D(i, i) for i in range(count)], 100)
    
    # Pool allocation
    print("\nPool Allocation:")
    pool_time = benchmark(lambda: cpp.batch.create_points_from_arrays(
        np.arange(count, dtype=np.float32),
        np.arange(count, dtype=np.float32)
    ), 100)
    
    print(f"Standard: {cpp_time:.3f} ms")
    print(f"Pool: {pool_time:.3f} ms")
    print(f"Pool is {format_speedup(pool_time, cpp_time)}")
    
    return cpp_time, pool_time

def bench_complex_scenario():
    """Benchmark a complex real-world scenario"""
    print("\n=== Complex Scenario (1000 shapes) ===")
    
    count = 1000
    
    def cpp_scenario():
        # Create shapes
        shapes = []
        for i in range(count):
            # Create rectangle
            points = [cpp.Point2D(0, 0), cpp.Point2D(10, 0), 
                     cpp.Point2D(10, 10), cpp.Point2D(0, 10)]
            
            # Transform
            t = cpp.Transform2D.translate(i * 0.1, i * 0.2)
            t = cpp.Transform2D.rotate(i * 0.01) @ t
            
            transformed = [t.transform_point(p) for p in points]
            
            # Create color
            color = cpp.Color(i % 256, (i * 2) % 256, (i * 3) % 256, 200)
            
            # Calculate bounds
            bbox = cpp.BoundingBox()
            for p in transformed:
                bbox.expand(p)
            
            shapes.append((transformed, color, bbox))
        
        # Composite colors
        canvas = cpp.Color(255, 255, 255, 255)
        for _, color, _ in shapes:
            canvas = color.blend_over(canvas)
    
    def py_scenario():
        # Create shapes
        shapes = []
        for i in range(count):
            # Create rectangle
            points = [PyPoint2D(0, 0), PyPoint2D(10, 0), 
                     PyPoint2D(10, 10), PyPoint2D(0, 10)]
            
            # Transform (simplified)
            angle = i * 0.01
            cos_a = math.cos(angle)
            sin_a = math.sin(angle)
            tx = i * 0.1
            ty = i * 0.2
            
            transformed = []
            for p in points:
                x = p.x * cos_a - p.y * sin_a + tx
                y = p.x * sin_a + p.y * cos_a + ty
                transformed.append(PyPoint2D(x, y))
            
            # Create color
            color = PyColor(i % 256, (i * 2) % 256, (i * 3) % 256, 200)
            
            shapes.append((transformed, color))
        
        # Composite colors
        canvas = PyColor(255, 255, 255, 255)
        for _, color in shapes:
            canvas = color.blend_over(canvas)
    
    cpp_time = benchmark(cpp_scenario, 10)
    py_time = benchmark(py_scenario, 10)
    
    print(f"C++: {cpp_time:.3f} ms")
    print(f"Python: {py_time:.3f} ms")
    print(f"C++ is {format_speedup(cpp_time, py_time)}")
    
    return cpp_time, py_time

def plot_results(results):
    """Plot benchmark results"""
    operations = list(results.keys())
    cpp_times = [r[0] for r in results.values()]
    py_times = [r[1] for r in results.values()]
    speedups = [py/cpp for cpp, py in zip(cpp_times, py_times)]
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6))
    
    # Time comparison
    x = np.arange(len(operations))
    width = 0.35
    
    ax1.bar(x - width/2, cpp_times, width, label='C++', color='blue')
    ax1.bar(x + width/2, py_times, width, label='Python', color='orange')
    ax1.set_xlabel('Operation')
    ax1.set_ylabel('Time (ms)')
    ax1.set_title('C++ vs Python Performance')
    ax1.set_xticks(x)
    ax1.set_xticklabels(operations, rotation=45, ha='right')
    ax1.legend()
    ax1.set_yscale('log')
    
    # Speedup chart
    colors = ['green' if s > 1 else 'red' for s in speedups]
    ax2.bar(operations, speedups, color=colors)
    ax2.axhline(y=1, color='black', linestyle='--', alpha=0.5)
    ax2.set_xlabel('Operation')
    ax2.set_ylabel('Speedup Factor')
    ax2.set_title('C++ Speedup vs Python')
    ax2.set_xticklabels(operations, rotation=45, ha='right')
    
    # Add value labels
    for i, v in enumerate(speedups):
        ax2.text(i, v + 0.1, f'{v:.1f}x', ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig('benchmark_results.png', dpi=150)
    print("\nResults saved to benchmark_results.png")

def main():
    """Run all benchmarks"""
    print("=== Claude Draw C++ vs Python Benchmarks ===")
    print("Running comprehensive performance tests...")
    
    results = {}
    
    # Run benchmarks
    results['Point Construction'] = bench_point_construction()
    results['Point Distance'] = bench_point_distance()
    results['Color Blending'] = bench_color_blending()
    results['Transform Point'] = bench_transform_point()
    results['Batch Ops'] = bench_batch_operations()
    results['Memory Alloc'] = bench_memory_allocation()
    results['Complex Scene'] = bench_complex_scenario()
    
    # Summary
    print("\n=== Summary ===")
    total_cpp = sum(r[0] for r in results.values())
    total_py = sum(r[1] for r in results.values())
    
    print(f"Total C++ time: {total_cpp:.3f} ms")
    print(f"Total Python time: {total_py:.3f} ms")
    print(f"Overall speedup: {total_py/total_cpp:.1f}x")
    
    # Plot results
    try:
        plot_results(results)
    except ImportError:
        print("\nMatplotlib not available - skipping plots")
    
    print("\n✅ Benchmark complete!")

if __name__ == "__main__":
    main()