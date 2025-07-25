#!/usr/bin/env python3
"""Test script to diagnose and fix batch operations"""

import sys
import os
import numpy as np

# Add build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build-release/src/bindings'))

try:
    import _claude_draw_cpp as cpp_module
    cpp = cpp_module.core
    batch = cpp_module.batch
except ImportError as e:
    print(f"Failed to import C++ bindings: {e}")
    sys.exit(1)

print("Testing batch operations...")

# Test 1: Simple batch creation
print("\n1. Testing batch creation:")
try:
    points = batch.create_points(10, 1.0, 2.0)
    print(f"  Created {points.shape} array")
    print(f"  First point: {points[0]}")
    print(f"  Array flags: C_CONTIGUOUS={points.flags['C_CONTIGUOUS']}, WRITEABLE={points.flags['WRITEABLE']}")
except Exception as e:
    print(f"  Error: {e}")

# Test 2: Transform with proper array
print("\n2. Testing transform with proper array:")
try:
    # Create a proper C-contiguous, writeable array
    points = np.array([[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]], dtype=np.float32)
    print(f"  Original points shape: {points.shape}")
    print(f"  Array flags: C_CONTIGUOUS={points.flags['C_CONTIGUOUS']}, WRITEABLE={points.flags['WRITEABLE']}")
    
    # Create transform
    transform = cpp.Transform2D.scale(2.0, 2.0)
    
    # Make sure array is writable and contiguous
    if not points.flags['C_CONTIGUOUS']:
        points = np.ascontiguousarray(points)
    if not points.flags['WRITEABLE']:
        points = points.copy()
    
    print(f"  Before transform: {points[0]}")
    batch.transform_points_batch(transform, points)
    print(f"  After transform: {points[0]}")
    print("  Success!")
except Exception as e:
    print(f"  Error: {e}")
    import traceback
    traceback.print_exc()

# Test 3: Batch distance calculation
print("\n3. Testing distance calculation:")
try:
    points1 = np.random.randn(5, 2).astype(np.float32)
    points2 = np.random.randn(5, 2).astype(np.float32)
    
    distances = batch.calculate_distances_batch(points1, points2)
    print(f"  Calculated {len(distances)} distances")
    print(f"  First distance: {distances[0]:.3f}")
    print("  Success!")
except Exception as e:
    print(f"  Error: {e}")

# Test 4: Color blending
print("\n4. Testing color blending:")
try:
    # Create color arrays
    fg_colors = np.array([0xFF0000FF, 0x00FF00FF, 0x0000FFFF], dtype=np.uint32)
    bg_colors = np.array([0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF], dtype=np.uint32)
    
    result = batch.blend_colors_batch(fg_colors, bg_colors)
    print(f"  Blended {len(result)} colors")
    print(f"  First result: 0x{result[0]:08X}")
    print("  Success!")
except Exception as e:
    print(f"  Error: {e}")

# Test 5: ContiguousBatch
print("\n5. Testing ContiguousBatch:")
try:
    point_batch = batch.PointBatch(100)
    
    # Add some points
    for i in range(10):
        point_batch.add(float(i), float(i * 2))
    
    print(f"  Added {point_batch.size()} points")
    
    # Get as array
    points_array = point_batch.as_array()
    print(f"  Array shape: {points_array.shape}")
    print(f"  First point: {points_array[0]}")
    print(f"  Array flags: C_CONTIGUOUS={points_array.flags['C_CONTIGUOUS']}, WRITEABLE={points_array.flags['WRITEABLE']}")
    print("  Success!")
except Exception as e:
    print(f"  Error: {e}")
    import traceback
    traceback.print_exc()

print("\nAll tests complete!")