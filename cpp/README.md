# Claude Draw C++ Implementation

High-performance C++ backend for the Claude Draw vector graphics library.

## Building

### Prerequisites

- CMake 3.20 or higher
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- Python development headers (for Python bindings)

### Quick Build

```bash
./build.sh
```

### Build Options

```bash
# Debug build with sanitizers
./build.sh --debug --sanitizers

# Release build without tests
./build.sh --no-tests

# Build with code coverage
./build.sh --coverage

# Clean rebuild
./build.sh --clean
```

### Manual Build

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Project Structure

```
cpp/
├── include/           # Public headers
│   └── claude_draw/
│       ├── core/      # Core types (Point2D, Color, etc.)
│       ├── shapes/    # Shape primitives
│       └── containers/# Container types
├── src/               # Implementation files
│   ├── core/          # Core type implementations
│   ├── shapes/        # Shape implementations
│   ├── containers/    # Container implementations
│   ├── memory/        # Memory management
│   ├── simd/          # SIMD optimizations
│   └── bindings/      # Python bindings
├── tests/             # Unit tests
├── benchmarks/        # Performance benchmarks
└── build/             # Build output directory
```

## Performance Goals

- Object creation: <0.5μs per object (50x improvement)
- Memory usage: ~40 bytes per object (50x reduction)
- Batch operations: 1M objects in <100ms
- SIMD-accelerated transformations

## Development Status

This is the C++ optimization layer for Claude Draw, currently under development.
See the main project README for more information.