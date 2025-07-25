#!/usr/bin/env python3
"""Test fast type conversions between Python and C++"""

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

def test_point2d_conversions():
    """Test Point2D type conversions"""
    print("\n=== Point2D Conversions ===")
    
    # Test construction from tuple
    p1 = cpp.Point2D((1.5, 2.5))
    assert p1.x == 1.5 and p1.y == 2.5, "Tuple construction failed"
    print("✓ Tuple construction")
    
    # Test construction from list
    p2 = cpp.Point2D([3.0, 4.0])
    assert p2.x == 3.0 and p2.y == 4.0, "List construction failed"
    print("✓ List construction")
    
    # Test construction from numpy array
    arr = np.array([5.0, 6.0], dtype=np.float32)
    p3 = cpp.Point2D(arr)
    assert p3.x == 5.0 and p3.y == 6.0, "Numpy array construction failed"
    print("✓ Numpy array construction")
    
    # Test buffer protocol
    p4 = cpp.Point2D(7.0, 8.0)
    arr2 = np.array(p4, copy=False)  # Zero-copy view
    assert arr2.shape == (2,), "Buffer protocol shape wrong"
    assert arr2[0] == 7.0 and arr2[1] == 8.0, "Buffer protocol values wrong"
    
    # Modify through numpy view
    arr2[0] = 9.0
    assert p4.x == 9.0, "Buffer protocol not sharing memory"
    print("✓ Buffer protocol (zero-copy)")
    
    # Test conversion back to Python
    t = tuple(p4)
    assert t == (9.0, 8.0), "Conversion to tuple failed"
    print("✓ Conversion to tuple")

def test_color_conversions():
    """Test Color type conversions"""
    print("\n=== Color Conversions ===")
    
    # Test construction from integer
    c1 = cpp.Color(0xFF0080FF)  # RGBA
    assert c1.r == 255 and c1.g == 0 and c1.b == 128 and c1.a == 255, "Integer construction failed"
    print("✓ Integer construction")
    
    # Test construction from tuple (integers)
    c2 = cpp.Color((255, 128, 64, 192))
    assert c2.r == 255 and c2.g == 128 and c2.b == 64 and c2.a == 192, "Tuple (int) construction failed"
    print("✓ Tuple (int) construction")
    
    # Test construction from tuple (floats)
    c3 = cpp.Color((1.0, 0.5, 0.25, 0.75))
    assert c3.r == 255 and c3.g == 127 and c3.b == 63 and c3.a == 191, "Tuple (float) construction failed"
    print("✓ Tuple (float) construction")
    
    # Test construction from object with attributes
    class ColorLike:
        def __init__(self, r, g, b, a=255):
            self.r = r
            self.g = g
            self.b = b
            self.a = a
    
    obj = ColorLike(100, 150, 200, 250)
    c4 = cpp.Color(obj)
    assert c4.r == 100 and c4.g == 150 and c4.b == 200 and c4.a == 250, "Object construction failed"
    print("✓ Object attribute construction")
    
    # Test conversion back to integer
    rgba = int(c4)
    assert rgba == c4.rgba, "Conversion to int failed"
    print("✓ Conversion to integer")

def test_transform2d_conversions():
    """Test Transform2D conversions"""
    print("\n=== Transform2D Conversions ===")
    
    # Test buffer protocol
    t = cpp.Transform2D.identity()
    arr = np.array(t, copy=False)  # Zero-copy view
    assert arr.shape == (3, 3), "Buffer protocol shape wrong"
    assert arr[0, 0] == 1.0 and arr[1, 1] == 1.0 and arr[2, 2] == 1.0, "Identity matrix wrong"
    
    # Modify through numpy view
    arr[0, 2] = 5.0  # Set translation x
    arr[1, 2] = 10.0  # Set translation y
    assert t[0, 2] == 5.0 and t[1, 2] == 10.0, "Buffer protocol not sharing memory"
    print("✓ Buffer protocol (zero-copy)")

def test_batch_conversions():
    """Test batch conversions performance"""
    print("\n=== Batch Conversion Performance ===")
    
    count = 100000
    
    # Test Point2D batch conversion
    tuples = [(float(i), float(i + 1)) for i in range(count)]
    
    start = time.time()
    points = [cpp.Point2D(t) for t in tuples]
    elapsed = (time.time() - start) * 1000  # ms
    
    print(f"Point2D conversions: {elapsed:.2f} ms for {count} points")
    print(f"  Per-point time: {elapsed * 1000 / count:.3f} µs")
    assert elapsed < 100, "Point2D conversion too slow"  # Should be < 100ms for 100k points
    
    # Test Color batch conversion  
    ints = [0xFF000000 | (i & 0xFFFFFF) for i in range(count)]
    
    start = time.time()
    colors = [cpp.Color(i) for i in ints]
    elapsed = (time.time() - start) * 1000  # ms
    
    print(f"Color conversions: {elapsed:.2f} ms for {count} colors")
    print(f"  Per-color time: {elapsed * 1000 / count:.3f} µs")
    assert elapsed < 100, "Color conversion too slow"  # Should be < 100ms for 100k colors

def test_numpy_batch_operations():
    """Test numpy batch operations"""
    print("\n=== Numpy Batch Operations ===")
    
    # Create arrays of points
    count = 10000
    points_a = np.random.rand(count, 2).astype(np.float32)
    points_b = np.random.rand(count, 2).astype(np.float32)
    result = np.zeros((count, 2), dtype=np.float32)
    
    # Test batch add
    start = time.time()
    cpp.batch_add_points(points_a, points_b, result)
    elapsed = (time.time() - start) * 1000
    
    print(f"Batch add: {elapsed:.2f} ms for {count} points")
    
    # Verify correctness
    expected = points_a + points_b
    assert np.allclose(result, expected), "Batch add incorrect"
    print("✓ Batch add correctness verified")
    
    # Test batch color blending
    colors_fg = np.random.randint(0, 0xFFFFFFFF, size=count, dtype=np.uint32)
    colors_bg = np.random.randint(0, 0xFFFFFFFF, size=count, dtype=np.uint32)
    blend_result = np.zeros(count, dtype=np.uint32)
    
    start = time.time()
    cpp.batch_blend_colors(colors_fg, colors_bg, blend_result)
    elapsed = (time.time() - start) * 1000
    
    print(f"Batch blend: {elapsed:.2f} ms for {count} colors")
    print("✓ Batch blend completed")
    
    # Test batch transform
    transform = cpp.Transform2D.translate(10, 20)
    points = np.random.rand(count, 2).astype(np.float32)
    transformed = np.zeros((count, 2), dtype=np.float32)
    
    start = time.time()
    cpp.batch_transform_points(transform, points, transformed)
    elapsed = (time.time() - start) * 1000
    
    print(f"Batch transform: {elapsed:.2f} ms for {count} points")
    
    # Verify correctness (first few points)
    for i in range(min(10, count)):
        p = cpp.Point2D(points[i])
        p_transformed = transform.transform_point(p)
        assert abs(transformed[i, 0] - p_transformed.x) < 1e-5, "Transform X incorrect"
        assert abs(transformed[i, 1] - p_transformed.y) < 1e-5, "Transform Y incorrect"
    print("✓ Batch transform correctness verified")
    
    # Test batch contains
    bb = cpp.BoundingBox(0, 0, 50, 50)
    test_points = np.random.rand(count, 2).astype(np.float32) * 100  # Some in, some out
    
    start = time.time()
    contains = cpp.batch_contains_points(bb, test_points)
    elapsed = (time.time() - start) * 1000
    
    print(f"Batch contains: {elapsed:.2f} ms for {count} points")
    assert contains.dtype == np.uint8, "Contains result should be uint8"
    assert len(contains) == count, "Contains result size wrong"
    print("✓ Batch contains completed")

def test_zero_copy_views():
    """Test zero-copy numpy views"""
    print("\n=== Zero-Copy Views ===")
    
    # Create a large array of Point2D objects in C++
    points = [cpp.Point2D(float(i), float(i * 2)) for i in range(1000)]
    
    # In a real implementation, we would have a way to get a numpy view
    # of the internal C++ vector without copying. For now, test the concept
    # with individual points
    
    p = cpp.Point2D(3.14, 2.71)
    view = np.array(p, copy=False)
    
    # Modify through view
    original_x = p.x
    view[0] = 99.9
    assert p.x == 99.9, "Zero-copy view failed"
    assert p.x != original_x, "Value didn't change"
    print("✓ Zero-copy view modification works")

def main():
    """Run all conversion tests"""
    print("=== C++ Type Conversion Tests ===")
    
    test_point2d_conversions()
    test_color_conversions()
    test_transform2d_conversions()
    test_batch_conversions()
    test_numpy_batch_operations()
    test_zero_copy_views()
    
    print("\n✅ All conversion tests passed!")

if __name__ == "__main__":
    main()