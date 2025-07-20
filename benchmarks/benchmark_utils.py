"""Utilities for performance benchmarking."""

import time
import gc
import psutil
import os
import statistics
from typing import List, Dict, Any, Callable, Optional, Tuple
from functools import wraps
from dataclasses import dataclass
import random

from claude_draw.shapes import Circle, Rectangle, Ellipse, Line
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color


@dataclass
class BenchmarkResult:
    """Container for benchmark results."""
    
    name: str
    execution_time: float  # seconds
    memory_before: float   # MB
    memory_after: float    # MB
    memory_peak: float     # MB
    memory_delta: float    # MB
    iterations: int
    objects_created: int
    time_per_object: float  # microseconds
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for reporting."""
        return {
            'name': self.name,
            'execution_time_seconds': self.execution_time,
            'memory_before_mb': self.memory_before,
            'memory_after_mb': self.memory_after,
            'memory_peak_mb': self.memory_peak,
            'memory_delta_mb': self.memory_delta,
            'iterations': self.iterations,
            'objects_created': self.objects_created,
            'time_per_object_microseconds': self.time_per_object,
            'objects_per_second': self.objects_created / self.execution_time if self.execution_time > 0 else 0
        }


class MemoryMonitor:
    """Monitor memory usage during benchmark execution."""
    
    def __init__(self):
        self.process = psutil.Process(os.getpid())
        self.peak_memory = 0
        self.start_memory = 0
    
    def start(self) -> float:
        """Start monitoring and return initial memory usage in MB."""
        gc.collect()  # Force garbage collection
        self.start_memory = self.process.memory_info().rss / 1024 / 1024
        self.peak_memory = self.start_memory
        return self.start_memory
    
    def update_peak(self) -> float:
        """Update peak memory usage and return current usage in MB."""
        current = self.process.memory_info().rss / 1024 / 1024
        self.peak_memory = max(self.peak_memory, current)
        return current
    
    def finish(self) -> Tuple[float, float]:
        """Finish monitoring and return (final_memory, peak_memory) in MB."""
        gc.collect()  # Force garbage collection
        final_memory = self.process.memory_info().rss / 1024 / 1024
        return final_memory, self.peak_memory


def benchmark_function(name: str, iterations: int = 1):
    """Decorator to benchmark a function's performance.
    
    Args:
        name: Name of the benchmark
        iterations: Number of iterations to run
    """
    def decorator(func: Callable) -> Callable:
        @wraps(func)
        def wrapper(*args, **kwargs) -> BenchmarkResult:
            monitor = MemoryMonitor()
            
            # Start monitoring
            memory_before = monitor.start()
            
            # Run benchmark
            start_time = time.perf_counter()
            
            objects_created = 0
            for i in range(iterations):
                result = func(*args, **kwargs)
                if isinstance(result, int):
                    objects_created += result
                elif isinstance(result, list):
                    objects_created += len(result)
                else:
                    objects_created += 1
                
                # Update peak memory every 100 iterations
                if i % 100 == 0:
                    monitor.update_peak()
            
            end_time = time.perf_counter()
            
            # Finish monitoring
            memory_after, memory_peak = monitor.finish()
            
            execution_time = end_time - start_time
            memory_delta = memory_after - memory_before
            time_per_object = (execution_time * 1_000_000) / objects_created if objects_created > 0 else 0
            
            return BenchmarkResult(
                name=name,
                execution_time=execution_time,
                memory_before=memory_before,
                memory_after=memory_after,
                memory_peak=memory_peak,
                memory_delta=memory_delta,
                iterations=iterations,
                objects_created=objects_created,
                time_per_object=time_per_object
            )
        
        return wrapper
    return decorator


class ShapeGenerator:
    """Generate random shapes for benchmarking."""
    
    def __init__(self, seed: Optional[int] = 42):
        """Initialize with optional random seed for reproducible results."""
        self.rng = random.Random(seed)
        self.colors = [
            Color.from_hex("#FF0000"),  # Red
            Color.from_hex("#00FF00"),  # Green
            Color.from_hex("#0000FF"),  # Blue
            Color.from_hex("#FFFF00"),  # Yellow
            Color.from_hex("#FF00FF"),  # Magenta
            Color.from_hex("#00FFFF"),  # Cyan
            Color.from_hex("#FFA500"),  # Orange
            Color.from_hex("#800080"),  # Purple
        ]
    
    def random_point(self, max_x: float = 1000, max_y: float = 1000) -> Point2D:
        """Generate a random point within bounds."""
        return Point2D(
            x=self.rng.uniform(0, max_x),
            y=self.rng.uniform(0, max_y)
        )
    
    def random_color(self) -> Color:
        """Get a random color from predefined set."""
        return self.rng.choice(self.colors)
    
    def create_random_circle(self) -> Circle:
        """Create a circle with random properties."""
        return Circle(
            center=self.random_point(),
            radius=self.rng.uniform(10, 100),
            fill=self.random_color(),
            stroke=self.random_color(),
            stroke_width=self.rng.uniform(1, 5)
        )
    
    def create_random_rectangle(self) -> Rectangle:
        """Create a rectangle with random properties."""
        x = self.rng.uniform(0, 900)
        y = self.rng.uniform(0, 900)
        return Rectangle(
            x=x,
            y=y,
            width=self.rng.uniform(20, 200),
            height=self.rng.uniform(20, 200),
            fill=self.random_color(),
            stroke=self.random_color(),
            stroke_width=self.rng.uniform(1, 5)
        )
    
    def create_random_ellipse(self) -> Ellipse:
        """Create an ellipse with random properties."""
        return Ellipse(
            center=self.random_point(),
            rx=self.rng.uniform(20, 100),
            ry=self.rng.uniform(20, 100),
            fill=self.random_color(),
            stroke=self.random_color(),
            stroke_width=self.rng.uniform(1, 5)
        )
    
    def create_random_line(self) -> Line:
        """Create a line with random properties."""
        return Line(
            start=self.random_point(),
            end=self.random_point(),
            stroke=self.random_color(),
            stroke_width=self.rng.uniform(1, 5)
        )
    
    def create_mixed_shapes(self, count: int) -> List[Any]:
        """Create a list of mixed random shapes.
        
        Args:
            count: Total number of shapes to create
            
        Returns:
            List of shape objects (25% each type)
        """
        shapes = []
        shapes_per_type = count // 4
        remainder = count % 4
        
        # Create equal amounts of each shape type
        for _ in range(shapes_per_type):
            shapes.append(self.create_random_circle())
            shapes.append(self.create_random_rectangle())
            shapes.append(self.create_random_ellipse())
            shapes.append(self.create_random_line())
        
        # Add remainder shapes
        shape_types = [
            self.create_random_circle,
            self.create_random_rectangle,
            self.create_random_ellipse,
            self.create_random_line
        ]
        
        for i in range(remainder):
            shapes.append(shape_types[i]())
        
        # Shuffle to mix the types
        self.rng.shuffle(shapes)
        return shapes


class BenchmarkSuite:
    """Collect and analyze benchmark results."""
    
    def __init__(self):
        self.results: List[BenchmarkResult] = []
    
    def add_result(self, result: BenchmarkResult):
        """Add a benchmark result to the suite."""
        self.results.append(result)
    
    def get_summary(self) -> Dict[str, Any]:
        """Get summary statistics for all benchmarks."""
        if not self.results:
            return {}
        
        execution_times = [r.execution_time for r in self.results]
        memory_deltas = [r.memory_delta for r in self.results]
        times_per_object = [r.time_per_object for r in self.results]
        objects_per_second = [r.objects_created / r.execution_time for r in self.results if r.execution_time > 0]
        
        return {
            'total_benchmarks': len(self.results),
            'total_objects_created': sum(r.objects_created for r in self.results),
            'execution_time_stats': {
                'mean': statistics.mean(execution_times),
                'median': statistics.median(execution_times),
                'min': min(execution_times),
                'max': max(execution_times),
                'stdev': statistics.stdev(execution_times) if len(execution_times) > 1 else 0
            },
            'memory_delta_stats': {
                'mean': statistics.mean(memory_deltas),
                'median': statistics.median(memory_deltas),
                'min': min(memory_deltas),
                'max': max(memory_deltas),
                'stdev': statistics.stdev(memory_deltas) if len(memory_deltas) > 1 else 0
            },
            'time_per_object_stats': {
                'mean': statistics.mean(times_per_object),
                'median': statistics.median(times_per_object),
                'min': min(times_per_object),
                'max': max(times_per_object),
                'stdev': statistics.stdev(times_per_object) if len(times_per_object) > 1 else 0
            },
            'objects_per_second_stats': {
                'mean': statistics.mean(objects_per_second) if objects_per_second else 0,
                'median': statistics.median(objects_per_second) if objects_per_second else 0,
                'min': min(objects_per_second) if objects_per_second else 0,
                'max': max(objects_per_second) if objects_per_second else 0,
                'stdev': statistics.stdev(objects_per_second) if len(objects_per_second) > 1 else 0
            }
        }
    
    def print_summary(self):
        """Print a formatted summary of benchmark results."""
        print("\n" + "="*80)
        print("BENCHMARK SUMMARY")
        print("="*80)
        
        for result in self.results:
            print(f"\n📊 {result.name}")
            print(f"   Objects Created: {result.objects_created:,}")
            print(f"   Execution Time: {result.execution_time:.4f} seconds")
            print(f"   Objects/Second: {result.objects_created/result.execution_time:,.0f}")
            print(f"   Time per Object: {result.time_per_object:.2f} μs")
            print(f"   Memory Delta: {result.memory_delta:+.2f} MB")
            print(f"   Peak Memory: {result.memory_peak:.2f} MB")
        
        summary = self.get_summary()
        if summary:
            print(f"\n📈 OVERALL STATISTICS")
            print(f"   Total Objects: {summary['total_objects_created']:,}")
            print(f"   Avg Time/Object: {summary['time_per_object_stats']['mean']:.2f} μs")
            print(f"   Avg Objects/Second: {summary['objects_per_second_stats']['mean']:,.0f}")
            print(f"   Avg Memory Delta: {summary['memory_delta_stats']['mean']:+.2f} MB")
        
        print("="*80)