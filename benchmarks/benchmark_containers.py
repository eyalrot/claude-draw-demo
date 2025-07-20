"""Benchmarks for container performance."""

from typing import List

from claude_draw.containers import Group, Layer, Drawing
from claude_draw.shapes import Circle, Rectangle, Ellipse, Line
from claude_draw.models.point import Point2D
from claude_draw.models.color import Color

from benchmark_utils import benchmark_function, ShapeGenerator, BenchmarkSuite


# Initialize shape generator
generator = ShapeGenerator(seed=42)


@benchmark_function("Add 1000 Shapes to Group", iterations=1)
def benchmark_group_1000_shapes() -> int:
    """Benchmark adding 1000 shapes to a Group container."""
    group = Group(name="test_group")
    shapes = generator.create_mixed_shapes(1000)
    
    for shape in shapes:
        group = group.add_child(shape)
    
    return len(group.children)


@benchmark_function("Add 1000 Shapes to Layer", iterations=1)
def benchmark_layer_1000_shapes() -> int:
    """Benchmark adding 1000 shapes to a Layer container."""
    layer = Layer(name="test_layer")
    shapes = generator.create_mixed_shapes(1000)
    
    for shape in shapes:
        layer = layer.add_child(shape)
    
    return len(layer.children)


@benchmark_function("Create Hierarchical Drawing (1000 shapes)", iterations=1)
def benchmark_hierarchical_drawing_1000() -> int:
    """Benchmark creating a hierarchical drawing with 1000 shapes."""
    drawing = Drawing(width=2000, height=2000, title="Benchmark Drawing")
    
    # Create 5 layers, each with 4 groups, each group with 50 shapes
    total_shapes = 0
    
    for layer_idx in range(5):
        layer = Layer(name=f"layer_{layer_idx}", z_index=layer_idx)
        
        for group_idx in range(4):
            group = Group(name=f"group_{layer_idx}_{group_idx}")
            shapes = generator.create_mixed_shapes(50)
            
            for shape in shapes:
                group = group.add_child(shape)
                total_shapes += 1
            
            layer = layer.add_child(group)
        
        drawing = drawing.add_child(layer)
    
    return total_shapes


@benchmark_function("Calculate Bounding Boxes (1000 shapes)", iterations=1)
def benchmark_bounding_boxes_1000() -> int:
    """Benchmark calculating bounding boxes for containers with 1000 shapes."""
    group = Group(name="bbox_test")
    shapes = generator.create_mixed_shapes(1000)
    
    # Add all shapes to group
    for shape in shapes:
        group = group.add_child(shape)
    
    # Calculate bounding box multiple times to test performance
    bounding_calculations = 0
    for _ in range(10):
        bbox = group.get_bounds()
        bounding_calculations += 1
    
    return len(group.children) * bounding_calculations


@benchmark_function("Group Operations (Add/Remove)", iterations=1)
def benchmark_group_operations() -> int:
    """Benchmark group add/remove operations."""
    group = Group(name="operations_test")
    shapes = generator.create_mixed_shapes(500)
    
    operations = 0
    
    # Add all shapes
    for shape in shapes:
        group = group.add_child(shape)
        operations += 1
    
    # Remove half the shapes
    for i, shape in enumerate(shapes[:250]):
        group = group.remove_child(shape.id)
        operations += 1
    
    # Add them back
    for shape in shapes[:250]:
        group = group.add_child(shape)
        operations += 1
    
    return operations


@benchmark_function("Layer Z-Index Sorting (1000 shapes)", iterations=1)
def benchmark_layer_sorting() -> int:
    """Benchmark layer z-index sorting with many children."""
    layer = Layer(name="sorting_test")
    
    # Create shapes and assign random z-indices
    shapes = generator.create_mixed_shapes(1000)
    operations = 0
    
    for i, shape in enumerate(shapes):
        # Simulate shapes with z-index by adding them in random order
        layer = layer.add_child(shape)
        operations += 1
    
    # Test getting sorted children multiple times
    for _ in range(10):
        # Since Layer doesn't have get_children_sorted, we'll simulate sorting
        sorted_children = sorted(layer.children, key=lambda child: getattr(child, 'z_index', 0))
        operations += len(sorted_children)
    
    return operations


def run_container_benchmarks() -> BenchmarkSuite:
    """Run all container benchmarks and return results."""
    suite = BenchmarkSuite()
    
    print("🏗️  Running Container Performance Benchmarks...")
    print("="*60)
    
    print("\n📦 Testing Container Operations...")
    
    result = benchmark_group_1000_shapes()
    suite.add_result(result)
    print(f"✅ Group (1000 shapes): {result.execution_time:.4f}s, {result.memory_delta:+.2f} MB")
    
    result = benchmark_layer_1000_shapes()
    suite.add_result(result)
    print(f"✅ Layer (1000 shapes): {result.execution_time:.4f}s, {result.memory_delta:+.2f} MB")
    
    result = benchmark_hierarchical_drawing_1000()
    suite.add_result(result)
    print(f"✅ Hierarchical drawing: {result.execution_time:.4f}s, {result.memory_delta:+.2f} MB")
    
    print("\n📐 Testing Computational Operations...")
    
    result = benchmark_bounding_boxes_1000()
    suite.add_result(result)
    print(f"✅ Bounding box calculations: {result.execution_time:.4f}s")
    
    result = benchmark_group_operations()
    suite.add_result(result)
    print(f"✅ Add/remove operations: {result.execution_time:.4f}s")
    
    result = benchmark_layer_sorting()
    suite.add_result(result)
    print(f"✅ Layer sorting: {result.execution_time:.4f}s")
    
    return suite


if __name__ == "__main__":
    suite = run_container_benchmarks()
    suite.print_summary()