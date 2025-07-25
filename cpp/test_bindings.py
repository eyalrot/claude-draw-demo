#!/usr/bin/env python3
"""Test the C++ Python bindings"""

import sys
import os

# Add build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build/src/bindings'))

try:
    import _claude_draw_cpp as cpp
    
    print("Claude Draw C++ Bindings Test")
    print("=============================")
    print(f"Version: {cpp.__version__}")
    print()
    
    # Test SIMD capabilities
    print("SIMD Capabilities:")
    print(f"  SSE2:    {cpp.has_sse2()}")
    print(f"  SSE3:    {cpp.has_sse3()}")
    print(f"  SSSE3:   {cpp.has_ssse3()}")
    print(f"  SSE4.1:  {cpp.has_sse41()}")
    print(f"  SSE4.2:  {cpp.has_sse42()}")
    print(f"  AVX:     {cpp.has_avx()}")
    print(f"  AVX2:    {cpp.has_avx2()}")
    print(f"  AVX512F: {cpp.has_avx512f()}")
    print(f"  FMA:     {cpp.has_fma()}")
    print()
    
    print(f"SIMD Summary: {cpp.get_simd_capabilities()}")
    print(f"SIMD Level: {cpp.get_simd_level()}")
    print()
    
    # Test other functions
    print(f"Memory usage: {cpp.get_memory_usage()} bytes")
    
    # Test submodules
    print("\nSubmodules:")
    print(f"  core:       {cpp.core}")
    print(f"  shapes:     {cpp.shapes}")
    print(f"  containers: {cpp.containers}")
    print(f"  memory:     {cpp.memory}")
    
    print("\nBindings test successful!")
    
except ImportError as e:
    print(f"Failed to import C++ bindings: {e}")
    print("Make sure to build with: cmake .. -DBUILD_PYTHON_BINDINGS=ON && make")
    sys.exit(1)