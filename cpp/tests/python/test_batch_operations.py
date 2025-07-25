#!/usr/bin/env python3
"""Test batch operations API"""

import sys
import os
import time
import numpy as np

# Add build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../build-release/src/bindings'))

try:
    import _claude_draw_cpp as cpp
except ImportError:
    print("Failed to import C++ bindings. Make sure to build first.")
    sys.exit(1)

def test_batch_create_points():
    """Test batch point creation"""
    print("\n=== Batch Point Creation ===")
    
    # Create uniform points
    count = 10000
    points = cpp.batch.create_points(count, 1.5, 2.5)
    
    assert isinstance(points, np.ndarray), "Should return numpy array"
    assert points.shape == (count, 2), f"Wrong shape: {points.shape}"
    assert points.dtype == np.float32, f"Wrong dtype: {points.dtype}"
    
    # Check values
    assert np.all(points[:, 0] == 1.5), "X coordinates wrong"
    assert np.all(points[:, 1] == 2.5), "Y coordinates wrong"
    print(f"✓ Created {count} uniform points")
    
    # Create from arrays
    x_coords = np.arange(1000, dtype=np.float32)
    y_coords = np.arange(1000, dtype=np.float32) * 2
    
    points_list = cpp.batch.create_points_from_arrays(x_coords, y_coords)
    assert len(points_list) == 1000, "Wrong number of points"
    
    # Verify first few points
    for i in range(10):
        assert points_list[i].x == float(i), f"Point {i} x wrong"
        assert points_list[i].y == float(i * 2), f"Point {i} y wrong"
    print("✓ Created points from coordinate arrays")

def test_batch_create_colors():
    """Test batch color creation"""
    print("\n=== Batch Color Creation ===")
    
    # Create uniform colors
    count = 10000
    test_color = 0xFF8040FF
    colors = cpp.batch.create_colors(count, test_color)
    
    assert isinstance(colors, np.ndarray), "Should return numpy array"
    assert colors.shape == (count,), f"Wrong shape: {colors.shape}"
    assert colors.dtype == np.uint32, f"Wrong dtype: {colors.dtype}"
    assert np.all(colors == test_color), "Color values wrong"
    print(f"✓ Created {count} uniform colors")
    
    # Create from component arrays
    r = np.full(1000, 255, dtype=np.uint8)
    g = np.full(1000, 128, dtype=np.uint8)
    b = np.full(1000, 64, dtype=np.uint8)
    a = np.full(1000, 255, dtype=np.uint8)
    
    colors_list = cpp.batch.create_colors_from_components(r, g, b, a)
    assert len(colors_list) == 1000, "Wrong number of colors"
    
    # Verify first few colors
    for i in range(10):
        assert colors_list[i].r == 255, f"Color {i} r wrong"
        assert colors_list[i].g == 128, f"Color {i} g wrong"
        assert colors_list[i].b == 64, f"Color {i} b wrong"
        assert colors_list[i].a == 255, f"Color {i} a wrong"
    print("✓ Created colors from component arrays")

def test_batch_transform_points():
    """Test batch point transformation"""
    print("\n=== Batch Transform Points ===")
    
    # Create test points
    count = 10000
    points = np.random.rand(count, 2).astype(np.float32) * 100
    original = points.copy()
    
    # Create transform
    transform = cpp.Transform2D.translate(10, 20)
    
    # Transform in place
    cpp.batch.transform_points_batch(transform, points)
    
    # Verify transformation
    expected = original + np.array([10, 20], dtype=np.float32)
    assert np.allclose(points, expected), "Transform failed"
    print(f"✓ Transformed {count} points")

def test_batch_blend_colors():
    """Test batch color blending"""
    print("\n=== Batch Color Blending ===")
    
    count = 10000
    
    # Create test colors
    fg_colors = np.full(count, 0x80FF0000, dtype=np.uint32)  # Semi-transparent red
    bg_colors = np.full(count, 0xFF0000FF, dtype=np.uint32)  # Opaque blue
    
    # Blend
    result = cpp.batch.blend_colors_batch(fg_colors, bg_colors)
    
    assert result.shape == (count,), "Wrong result shape"
    assert result.dtype == np.uint32, "Wrong result dtype"
    
    # Check first result
    first = cpp.Color(int(result[0]))
    assert first.a == 255, "Alpha should be opaque"
    assert first.r > 0, "Should have some red"
    assert first.b > 0, "Should have some blue"
    print(f"✓ Blended {count} color pairs")

def test_batch_contains_points():
    """Test batch point containment"""
    print("\n=== Batch Contains Points ===")
    
    # Create bounding box
    box = cpp.BoundingBox(0, 0, 10, 10)
    
    # Create test points
    count = 10000
    points = np.zeros((count, 2), dtype=np.float32)
    
    # Half inside, half outside
    points[:count//2] = [5, 5]    # Inside
    points[count//2:] = [15, 15]  # Outside
    
    # Test containment
    result = cpp.batch.contains_points_batch(box, points)
    
    assert result.shape == (count,), "Wrong result shape"
    assert result.dtype == np.bool_, "Wrong result dtype"
    
    # Verify
    assert np.all(result[:count//2] == True), "First half should be inside"
    assert np.all(result[count//2:] == False), "Second half should be outside"
    print(f"✓ Tested containment for {count} points")

def test_batch_calculate_distances():
    """Test batch distance calculation"""
    print("\n=== Batch Distance Calculation ===")
    
    count = 10000
    
    # Create test points
    points1 = np.zeros((count, 2), dtype=np.float32)
    points2 = np.full((count, 2), [3, 4], dtype=np.float32)
    
    # Calculate distances
    distances = cpp.batch.calculate_distances_batch(points1, points2)
    
    assert distances.shape == (count,), "Wrong result shape"
    assert distances.dtype == np.float32, "Wrong result dtype"
    
    # All distances should be 5.0 (3-4-5 triangle)
    assert np.allclose(distances, 5.0), "Distance calculation wrong"
    print(f"✓ Calculated {count} distances")

def test_contiguous_batches():
    """Test contiguous batch containers"""
    print("\n=== Contiguous Batches ===")
    
    # Test PointBatch
    point_batch = cpp.batch.PointBatch(1000)
    
    # Add points
    for i in range(100):
        point_batch.add(float(i), float(i * 2))
    
    assert point_batch.size() == 100, "Wrong size"
    
    # Get as array
    arr = point_batch.as_array()
    assert arr.shape == (100, 2), "Wrong array shape"
    assert arr[0, 0] == 0.0 and arr[0, 1] == 0.0, "First point wrong"
    assert arr[99, 0] == 99.0 and arr[99, 1] == 198.0, "Last point wrong"
    print("✓ PointBatch works correctly")
    
    # Test ColorBatch
    color_batch = cpp.batch.ColorBatch(1000)
    
    # Add colors
    for i in range(100):
        color_batch.add_rgb(i % 256, (i * 2) % 256, (i * 3) % 256)
    
    assert color_batch.size() == 100, "Wrong size"
    
    # Get as array
    arr = color_batch.as_array()
    assert arr.shape == (100,), "Wrong array shape"
    assert arr.dtype == np.uint32, "Wrong dtype"
    print("✓ ColorBatch works correctly")

def test_performance():
    """Test batch operation performance"""
    print("\n=== Batch Performance ===")
    
    count = 100000
    
    # Test point creation performance
    start = time.time()
    points = cpp.batch.create_points(count, 0, 0)
    batch_time = (time.time() - start) * 1000  # ms
    
    # Compare with loop
    start = time.time()
    points_list = []
    for i in range(min(count, 1000)):  # Only do 1000 for comparison
        points_list.append(cpp.Point2D(0, 0))
    loop_time_per_1000 = (time.time() - start) * 1000
    estimated_loop_time = loop_time_per_1000 * (count / 1000)
    
    print(f"Batch creation: {batch_time:.2f} ms for {count} points")
    print(f"Loop creation (estimated): {estimated_loop_time:.2f} ms for {count} points")
    print(f"Speedup: {estimated_loop_time / batch_time:.1f}x")
    
    # Test transform performance
    points = np.random.rand(count, 2).astype(np.float32)
    transform = cpp.Transform2D.translate(10, 20)
    
    start = time.time()
    cpp.batch.transform_points_batch(transform, points)
    transform_time = (time.time() - start) * 1000
    
    print(f"Batch transform: {transform_time:.2f} ms for {count} points")
    print(f"Points per ms: {count / transform_time:.0f}")

def main():
    """Run all batch operation tests"""
    print("=== C++ Batch Operations Tests ===")
    
    test_batch_create_points()
    test_batch_create_colors()
    test_batch_transform_points()
    test_batch_blend_colors()
    test_batch_contains_points()
    test_batch_calculate_distances()
    test_contiguous_batches()
    test_performance()
    
    print("\n✅ All batch operation tests passed!")

if __name__ == "__main__":
    main()