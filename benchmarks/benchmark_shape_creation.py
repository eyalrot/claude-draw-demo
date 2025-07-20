"""Benchmarks for shape creation performance."""

import time
from typing import List

from claude_draw.shapes import Circle, Rectangle, Ellipse, Line
from claude_draw.factories import create_circle, create_rectangle, create_ellipse, create_line
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color

from benchmark_utils import benchmark_function, ShapeGenerator, BenchmarkSuite, BenchmarkResult


# Initialize shape generator with consistent seed for reproducible results
generator = ShapeGenerator(seed=42)


@benchmark_function("Create 1000 Mixed Shapes (Direct)", iterations=1)
def benchmark_1000_shapes_direct() -> int:
    """Benchmark creating 1000 mixed shapes using direct instantiation."""
    shapes = []
    
    # Create 250 of each shape type
    for i in range(250):
        # Circle
        shapes.append(Circle(
            center=Point2D(x=i, y=i),
            radius=10 + (i % 50),
            fill=Color.from_rgb(255, 0, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Rectangle
        shapes.append(Rectangle(
            x=i, y=i,
            width=20 + (i % 30),
            height=15 + (i % 25),
            fill=Color.from_rgb(0, 255, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Ellipse
        shapes.append(Ellipse(
            center=Point2D(x=i, y=i),
            rx=15 + (i % 35),
            ry=10 + (i % 30),
            fill=Color.from_rgb(0, 0, 255),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Line
        shapes.append(Line(
            start=Point2D(x=i, y=i),
            end=Point2D(x=i+10, y=i+10),
            stroke=Color.from_rgb(255, 255, 0),
            stroke_width=1.0
        ))
    
    return len(shapes)


@benchmark_function("Create 1000 Mixed Shapes (Factory)", iterations=1)
def benchmark_1000_shapes_factory() -> int:
    """Benchmark creating 1000 mixed shapes using factory functions."""
    shapes = []
    
    # Create 250 of each shape type using factory functions
    for i in range(250):
        # Circle
        shapes.append(create_circle(
            x=i, y=i,
            radius=10 + (i % 50),
            fill=Color.from_rgb(255, 0, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Rectangle
        shapes.append(create_rectangle(
            x=i, y=i,
            width=20 + (i % 30),
            height=15 + (i % 25),
            fill=Color.from_rgb(0, 255, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Ellipse
        shapes.append(create_ellipse(
            x=i, y=i,
            rx=15 + (i % 35),
            ry=10 + (i % 30),
            fill=Color.from_rgb(0, 0, 255),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Line
        shapes.append(create_line(
            x1=i, y1=i,
            x2=i+10, y2=i+10,
            stroke=Color.from_rgb(255, 255, 0),
            stroke_width=1.0
        ))
    
    return len(shapes)


@benchmark_function("Create 1000 Mixed Shapes (Random)", iterations=1)
def benchmark_1000_shapes_random() -> int:
    """Benchmark creating 1000 mixed shapes with random properties."""
    shapes = generator.create_mixed_shapes(1000)
    return len(shapes)


@benchmark_function("Create 5000 Mixed Shapes (Direct)", iterations=1)
def benchmark_5000_shapes_direct() -> int:
    """Benchmark creating 5000 mixed shapes using direct instantiation."""
    shapes = []
    
    # Create 1250 of each shape type
    for i in range(1250):
        # Circle
        shapes.append(Circle(
            center=Point2D(x=i % 1000, y=(i // 1000) * 100),
            radius=10 + (i % 50),
            fill=Color.from_rgb(255, 0, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Rectangle
        shapes.append(Rectangle(
            x=i % 1000, y=(i // 1000) * 100,
            width=20 + (i % 30),
            height=15 + (i % 25),
            fill=Color.from_rgb(0, 255, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Ellipse
        shapes.append(Ellipse(
            center=Point2D(x=i % 1000, y=(i // 1000) * 100),
            rx=15 + (i % 35),
            ry=10 + (i % 30),
            fill=Color.from_rgb(0, 0, 255),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Line
        shapes.append(Line(
            start=Point2D(x=i % 1000, y=(i // 1000) * 100),
            end=Point2D(x=(i % 1000)+10, y=((i // 1000) * 100)+10),
            stroke=Color.from_rgb(255, 255, 0),
            stroke_width=1.0
        ))
    
    return len(shapes)


@benchmark_function("Create 5000 Mixed Shapes (Factory)", iterations=1)
def benchmark_5000_shapes_factory() -> int:
    """Benchmark creating 5000 mixed shapes using factory functions."""
    shapes = []
    
    # Create 1250 of each shape type using factory functions
    for i in range(1250):
        # Circle
        shapes.append(create_circle(
            x=i % 1000, y=(i // 1000) * 100,
            radius=10 + (i % 50),
            fill=Color.from_rgb(255, 0, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Rectangle
        shapes.append(create_rectangle(
            x=i % 1000, y=(i // 1000) * 100,
            width=20 + (i % 30),
            height=15 + (i % 25),
            fill=Color.from_rgb(0, 255, 0),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Ellipse
        shapes.append(create_ellipse(
            x=i % 1000, y=(i // 1000) * 100,
            rx=15 + (i % 35),
            ry=10 + (i % 30),
            fill=Color.from_rgb(0, 0, 255),
            stroke=Color.from_rgb(0, 0, 0),
            stroke_width=1.0
        ))
        
        # Line
        shapes.append(create_line(
            x1=i % 1000, y1=(i // 1000) * 100,
            x2=(i % 1000)+10, y2=((i // 1000) * 100)+10,
            stroke=Color.from_rgb(255, 255, 0),
            stroke_width=1.0
        ))
    
    return len(shapes)


@benchmark_function("Create 5000 Mixed Shapes (Random)", iterations=1)
def benchmark_5000_shapes_random() -> int:
    """Benchmark creating 5000 mixed shapes with random properties."""
    shapes = generator.create_mixed_shapes(5000)
    return len(shapes)


def run_shape_creation_benchmarks() -> BenchmarkSuite:
    """Run all shape creation benchmarks and return results."""
    suite = BenchmarkSuite()
    
    print("🚀 Running Shape Creation Benchmarks...")
    print("="*60)
    
    # 1000 shapes benchmarks
    print("\n📊 Testing 1000 Mixed Shapes...")
    
    result = benchmark_1000_shapes_direct()
    suite.add_result(result)
    print(f"✅ Direct instantiation: {result.execution_time:.4f}s, {result.objects_created/result.execution_time:.0f} shapes/sec")
    
    result = benchmark_1000_shapes_factory()
    suite.add_result(result)
    print(f"✅ Factory functions: {result.execution_time:.4f}s, {result.objects_created/result.execution_time:.0f} shapes/sec")
    
    result = benchmark_1000_shapes_random()
    suite.add_result(result)
    print(f"✅ Random properties: {result.execution_time:.4f}s, {result.objects_created/result.execution_time:.0f} shapes/sec")
    
    # 5000 shapes benchmarks
    print("\n📊 Testing 5000 Mixed Shapes...")
    
    result = benchmark_5000_shapes_direct()
    suite.add_result(result)
    print(f"✅ Direct instantiation: {result.execution_time:.4f}s, {result.objects_created/result.execution_time:.0f} shapes/sec")
    
    result = benchmark_5000_shapes_factory()
    suite.add_result(result)
    print(f"✅ Factory functions: {result.execution_time:.4f}s, {result.objects_created/result.execution_time:.0f} shapes/sec")
    
    result = benchmark_5000_shapes_random()
    suite.add_result(result)
    print(f"✅ Random properties: {result.execution_time:.4f}s, {result.objects_created/result.execution_time:.0f} shapes/sec")
    
    return suite


if __name__ == "__main__":
    suite = run_shape_creation_benchmarks()
    suite.print_summary()