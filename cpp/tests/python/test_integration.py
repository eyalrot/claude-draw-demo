#!/usr/bin/env python3
"""Integration tests for C++ bindings - testing all components together"""

import sys
import os
import time
import numpy as np
import math

# Add build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../build-release/src/bindings'))

try:
    import _claude_draw_cpp as cpp
except ImportError:
    print("Failed to import C++ bindings. Make sure to build first.")
    sys.exit(1)

def test_transform_chain():
    """Test complex transformation chains"""
    print("\n=== Transform Chain Integration ===")
    
    # Create point
    p = cpp.Point2D(10.0, 20.0)
    
    # Create transformation chain
    t1 = cpp.Transform2D.translate(5.0, 10.0)
    t2 = cpp.Transform2D.rotate(math.pi / 4)  # 45 degrees
    t3 = cpp.Transform2D.scale(2.0, 2.0)
    t4 = cpp.Transform2D.translate(-5.0, -10.0)
    
    # Compose transforms
    combined = t4 @ t3 @ t2 @ t1  # Matrix multiplication
    
    # Apply to point
    result = combined.transform_point(p)
    
    # Apply step by step for verification
    step1 = t1.transform_point(p)
    step2 = t2.transform_point(step1)
    step3 = t3.transform_point(step2)
    step4 = t4.transform_point(step3)
    
    # Should get same result
    assert abs(result.x - step4.x) < 1e-5, f"X mismatch: {result.x} vs {step4.x}"
    assert abs(result.y - step4.y) < 1e-5, f"Y mismatch: {result.y} vs {step4.y}"
    print("✓ Transform chain produces consistent results")

def test_color_compositing():
    """Test color blending with multiple layers"""
    print("\n=== Color Compositing Integration ===")
    
    # Create layer stack
    background = cpp.Color(255, 255, 255, 255)  # White
    layer1 = cpp.Color(255, 0, 0, 128)          # Semi-transparent red
    layer2 = cpp.Color(0, 255, 0, 128)          # Semi-transparent green
    layer3 = cpp.Color(0, 0, 255, 64)           # Quarter-transparent blue
    
    # Composite layers
    result1 = layer1.blend_over(background)
    result2 = layer2.blend_over(result1)
    result3 = layer3.blend_over(result2)
    
    # Final color should have components from all layers
    assert result3.r > 0, "Should have red component"
    assert result3.g > 0, "Should have green component"
    assert result3.b > 0, "Should have blue component"
    assert result3.a == 255, "Should be opaque"
    
    print(f"✓ Composited color: RGB({result3.r}, {result3.g}, {result3.b})")

def test_batch_transform_bbox():
    """Test batch operations with transforms and bounding boxes"""
    print("\n=== Batch Transform + BBox Integration ===")
    
    # Create batch of random points
    count = 10000
    points = np.random.uniform(-50, 50, size=(count, 2)).astype(np.float32)
    
    # Calculate original bounding box
    min_x, min_y = points.min(axis=0)
    max_x, max_y = points.max(axis=0)
    original_bbox = cpp.BoundingBox(min_x, min_y, max_x, max_y)
    
    print(f"Original bounds: ({min_x:.2f}, {min_y:.2f}) to ({max_x:.2f}, {max_y:.2f})")
    
    # Transform all points
    transform = cpp.Transform2D.rotate(math.pi / 3)
    transform = cpp.Transform2D.scale(1.5, 0.8) @ transform
    transform = cpp.Transform2D.translate(10, -20) @ transform
    
    cpp.batch.transform_points_batch(transform, points)
    
    # Calculate new bounding box
    min_x, min_y = points.min(axis=0)
    max_x, max_y = points.max(axis=0)
    transformed_bbox = cpp.BoundingBox(min_x, min_y, max_x, max_y)
    
    print(f"Transformed bounds: ({min_x:.2f}, {min_y:.2f}) to ({max_x:.2f}, {max_y:.2f})")
    
    # Test containment
    contains = cpp.batch.contains_points_batch(transformed_bbox, points)
    assert np.all(contains), "All points should be contained in bbox"
    print(f"✓ All {count} transformed points contained in new bbox")

def test_numpy_interop():
    """Test numpy array interoperability"""
    print("\n=== NumPy Interop Integration ===")
    
    # Create points in numpy
    count = 1000
    np_points = np.random.randn(count, 2).astype(np.float32) * 10
    
    # Convert to C++ and back
    cpp_points = []
    for i in range(count):
        cpp_points.append(cpp.Point2D(np_points[i]))
    
    # Modify through C++
    transform = cpp.Transform2D.scale(2.0, 3.0)
    for i, p in enumerate(cpp_points):
        cpp_points[i] = transform.transform_point(p)
    
    # Convert back to numpy
    result = np.zeros((count, 2), dtype=np.float32)
    for i, p in enumerate(cpp_points):
        result[i] = np.array(p, copy=False)
    
    # Verify transformation
    expected = np_points * np.array([2.0, 3.0])
    assert np.allclose(result, expected, rtol=1e-5), "Transformation mismatch"
    print("✓ NumPy arrays correctly transformed through C++")

def test_memory_efficiency():
    """Test memory-efficient batch operations"""
    print("\n=== Memory Efficiency Integration ===")
    
    # Use contiguous batches for efficiency
    point_batch = cpp.batch.PointBatch(10000)
    color_batch = cpp.batch.ColorBatch(10000)
    
    # Fill batches
    for i in range(10000):
        point_batch.add(float(i % 100), float(i % 200))
        color_batch.add_rgb(i % 256, (i * 2) % 256, (i * 3) % 256)
    
    # Get as numpy arrays (zero-copy views)
    points_array = point_batch.as_array()
    colors_array = color_batch.as_array()
    
    assert points_array.shape == (10000, 2), "Wrong points shape"
    assert colors_array.shape == (10000,), "Wrong colors shape"
    
    # Verify data integrity
    assert points_array[0, 0] == 0.0, "First point X wrong"
    assert points_array[9999, 0] == 99.0, "Last point X wrong"
    
    # Modify through numpy (should affect C++ data)
    points_array[:, 0] *= 2.0
    
    # Verify modification propagated
    modified_array = point_batch.as_array()
    assert modified_array[0, 0] == 0.0, "First point should still be 0"
    assert modified_array[1, 0] == 2.0, "Second point should be doubled"
    
    print("✓ Contiguous batches provide efficient zero-copy access")

def test_real_world_rendering():
    """Simulate a real-world rendering scenario"""
    print("\n=== Real-World Rendering Simulation ===")
    
    # Create a scene with multiple shapes
    shapes = []
    
    # Generate 50 random rectangles
    np.random.seed(42)
    for i in range(50):
        # Rectangle corners
        w = np.random.uniform(5, 20)
        h = np.random.uniform(5, 20)
        vertices = [
            cpp.Point2D(0, 0),
            cpp.Point2D(w, 0),
            cpp.Point2D(w, h),
            cpp.Point2D(0, h)
        ]
        
        # Random transform
        x = np.random.uniform(-100, 100)
        y = np.random.uniform(-100, 100)
        angle = np.random.uniform(0, 2 * math.pi)
        scale = np.random.uniform(0.5, 2.0)
        
        transform = cpp.Transform2D.translate(x, y)
        transform = cpp.Transform2D.rotate(angle) @ transform
        transform = cpp.Transform2D.scale(scale, scale) @ transform
        
        # Random color
        color = cpp.Color(
            int(np.random.randint(0, 256)),
            int(np.random.randint(0, 256)),
            int(np.random.randint(0, 256)),
            200  # Slightly transparent
        )
        
        # Calculate bounding box
        bbox = cpp.BoundingBox()
        transformed_vertices = []
        for v in vertices:
            tv = transform.transform_point(v)
            transformed_vertices.append(tv)
            bbox.expand(tv)
        
        shapes.append({
            'vertices': transformed_vertices,
            'color': color,
            'bbox': bbox
        })
    
    # Find overlapping shapes
    overlaps = 0
    for i in range(len(shapes)):
        for j in range(i + 1, len(shapes)):
            if shapes[i]['bbox'].intersects(shapes[j]['bbox']):
                overlaps += 1
    
    print(f"Found {overlaps} overlapping shape pairs")
    
    # Simulate compositing
    canvas = cpp.Color(255, 255, 255, 255)  # White background
    
    for shape in shapes:
        # In real renderer, this would be per-pixel
        canvas = shape['color'].blend_over(canvas)
    
    # Canvas should have mixed colors
    assert canvas.r < 255, "Canvas should not be pure white"
    assert canvas.g < 255, "Canvas should not be pure white"
    assert canvas.b < 255, "Canvas should not be pure white"
    
    print(f"✓ Final canvas color: RGB({canvas.r}, {canvas.g}, {canvas.b})")

def test_performance_comparison():
    """Compare C++ performance with pure Python"""
    print("\n=== Performance Comparison ===")
    
    count = 100000
    
    # Test 1: Point transformation
    points = np.random.randn(count, 2).astype(np.float32)
    transform = cpp.Transform2D.rotate(0.5)
    transform = cpp.Transform2D.scale(1.5, 2.0) @ transform
    
    # C++ batch transform
    cpp_points = points.copy()
    start = time.time()
    cpp.batch.transform_points_batch(transform, cpp_points)
    cpp_time = time.time() - start
    
    # Python equivalent (simplified)
    py_points = points.copy()
    angle = 0.5
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)
    start = time.time()
    for i in range(len(py_points)):
        x, y = py_points[i]
        # Rotate
        rx = x * cos_a - y * sin_a
        ry = x * sin_a + y * cos_a
        # Scale
        py_points[i, 0] = rx * 1.5
        py_points[i, 1] = ry * 2.0
    py_time = time.time() - start
    
    print(f"Transform {count} points:")
    print(f"  C++ batch: {cpp_time*1000:.2f} ms")
    print(f"  Python loop: {py_time*1000:.2f} ms")
    print(f"  Speedup: {py_time/cpp_time:.1f}x")
    
    # Test 2: Distance calculation
    points1 = np.random.randn(count//10, 2).astype(np.float32)
    points2 = np.random.randn(count//10, 2).astype(np.float32)
    
    # C++ batch distance
    start = time.time()
    cpp_distances = cpp.batch.calculate_distances_batch(points1, points2)
    cpp_time = time.time() - start
    
    # NumPy distance
    start = time.time()
    np_distances = np.sqrt(((points1 - points2) ** 2).sum(axis=1))
    np_time = time.time() - start
    
    print(f"\nCalculate {count//10} distances:")
    print(f"  C++ batch: {cpp_time*1000:.2f} ms")
    print(f"  NumPy: {np_time*1000:.2f} ms")
    
    # Verify correctness
    assert np.allclose(cpp_distances, np_distances, rtol=1e-5), "Distance mismatch"

def test_edge_cases():
    """Test edge cases and error handling"""
    print("\n=== Edge Cases Integration ===")
    
    # Empty bounding box
    empty_box = cpp.BoundingBox()
    assert empty_box.is_empty(), "Default bbox should be empty"
    
    # Degenerate transform
    degenerate = cpp.Transform2D.scale(0.0, 1.0)
    assert not degenerate.is_invertible(), "Should not be invertible"
    
    # Zero-alpha blending
    transparent = cpp.Color(255, 0, 0, 0)
    opaque = cpp.Color(0, 0, 255, 255)
    result = transparent.blend_over(opaque)
    assert result.rgba == opaque.rgba, "Transparent should have no effect"
    
    # Very small distances
    p1 = cpp.Point2D(0.0, 0.0)
    p2 = cpp.Point2D(1e-7, 1e-7)
    dist = p1.distance_to(p2)
    assert dist > 0, "Distance should be positive"
    assert dist < 1e-6, "Distance should be very small"
    
    # Large batch allocation
    try:
        huge_batch = cpp.batch.create_points(1000000, 0, 0)
        assert huge_batch.shape[0] == 1000000, "Should create 1M points"
        print("✓ Successfully allocated 1M points")
    except MemoryError:
        print("✓ Large allocation failed gracefully")
    
    print("✓ All edge cases handled correctly")

def main():
    """Run all integration tests"""
    print("=== C++ Integration Tests ===")
    
    test_transform_chain()
    test_color_compositing()
    test_batch_transform_bbox()
    test_numpy_interop()
    test_memory_efficiency()
    test_real_world_rendering()
    test_performance_comparison()
    test_edge_cases()
    
    print("\n✅ All integration tests passed!")

if __name__ == "__main__":
    main()