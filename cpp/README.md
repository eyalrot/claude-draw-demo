# Claude Draw C++ Implementation

High-performance C++ backend for the Claude Draw vector graphics library, providing 50-100x performance improvements for handling millions of graphics objects.

## Features

- **Ultra-Fast Object Creation**: <0.5μs per object (50x faster than Python)
- **Memory Efficient**: ~40 bytes per object (50x reduction from Python)
- **SIMD Optimized**: Hardware-accelerated operations using AVX2/AVX512
- **Zero-Copy Python Integration**: Direct memory sharing with NumPy
- **Spatial Indexing**: Fast viewport queries and hit testing
- **Modern C++20**: Clean, efficient implementation

## Quick Start

```bash
# Build everything
make

# Run tests
make test

# Run benchmarks
make bench

# See all available commands
make help
```

## Building

### Prerequisites

- CMake 3.20 or higher
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- Python 3.8+ with development headers (for Python bindings)
- Optional: ninja-build for faster builds

### Build Options

```bash
# Debug build
make debug

# Release build with optimizations
make release

# Build with sanitizers
make sanitize

# Generate coverage report
make coverage-html

# Build only Python bindings
make python
```

### Using the Build Script

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

### Manual CMake Build

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
make -j$(nproc)
```

## Project Structure

```
cpp/
├── include/claude_draw/    # Public headers
│   ├── core/              # Core types (Point2D, Color, Transform2D, BoundingBox)
│   ├── shapes/            # Shape primitives (Circle, Rectangle, etc.)
│   └── containers/        # Container types (Group, Layer, Drawing)
├── src/                   # Implementation files
│   ├── core/              # Core type implementations with SIMD
│   ├── shapes/            # Shape implementations
│   ├── containers/        # Container implementations
│   ├── memory/            # Custom allocators and pools
│   ├── simd/              # SIMD-specific optimizations
│   └── bindings/          # Python bindings (pybind11)
├── tests/                 # Unit tests (Google Test)
├── benchmarks/            # Performance benchmarks (Google Benchmark)
├── scripts/               # Utility scripts
├── Makefile              # Main build system
└── CMakeLists.txt        # CMake configuration
```

## Development Workflow

### Running Tests

```bash
# Run all tests
make test

# Run tests with verbose output
make test-verbose

# Run specific test suite
./build/tests/claude_draw_tests --gtest_filter="Point2D*"

# Run with memory checking
make memcheck

# Generate coverage report
make coverage-html
# Open coverage/index.html in browser
```

### Code Quality

```bash
# Format code
make format

# Check formatting
make format-check

# Run static analysis
make lint

# Run all checks (tests + lint + format)
make check
```

### Benchmarking

```bash
# Run benchmarks
make bench

# Run with custom iterations
./build-release/benchmarks/claude_draw_benchmarks --benchmark_min_time=2s

# Profile with perf (Linux)
make perf
```

### Python Integration

```bash
# Build Python module
make python

# Test Python bindings
make test-python

# Install Python module
make install-python
```

## Performance

Current performance metrics on modern hardware (Intel i9, 32GB RAM):

| Operation | Pure Python | C++ Backend | Speedup |
|-----------|-------------|-------------|---------|
| Create Circle | ~24μs | <0.5μs | 50x |
| Transform 1K Points | ~15ms | <0.2ms | 75x |
| Query 10K from 1M | ~250ms | <1ms | 250x |
| Memory per Object | ~2KB | ~40 bytes | 50x |

## API Example

### C++ API

```cpp
#include <claude_draw/core/point2d.h>
#include <claude_draw/core/color.h>
#include <claude_draw/shapes/circle.h>

using namespace claude_draw;

// Create shapes
Point2D center(100.0f, 100.0f);
Color fill = Color::from_hex(0x3498db);
Circle circle(center, 50.0f, fill);

// Batch operations
std::vector<Point2D> points(1000);
Transform2D transform = Transform2D::rotate(0.785f).scale(1.5f);
batch::transform_points(transform, points.data(), points.data(), points.size());
```

### Python API

```python
import claude_draw_cpp as cpp

# Create shapes using C++ backend
circle = cpp.Circle(center=(100, 100), radius=50)
rect = cpp.Rectangle(x=0, y=0, width=200, height=100)

# Batch operations with NumPy
import numpy as np
points = np.random.rand(1000000, 2).astype(np.float32)
transform = cpp.Transform2D.rotate(0.785).scale(1.5)
transformed = cpp.batch_transform_points(transform, points)
```

## Architecture Highlights

- **Memory Layout**: Cache-friendly POD structures, 32-byte aligned for SIMD
- **Batch Processing**: APIs designed for operating on thousands of objects
- **Zero-Copy**: Direct memory sharing between C++ and Python/NumPy
- **Spatial Indexing**: R-tree for fast viewport queries
- **Custom Allocators**: Arena allocators for minimal allocation overhead

For detailed architecture information, see [ARCHITECTURE.md](ARCHITECTURE.md).

## CI/CD

The project uses GitHub Actions for continuous integration:

- **Build & Test**: Ubuntu and macOS, multiple compiler versions
- **Sanitizers**: AddressSanitizer and UndefinedBehaviorSanitizer
- **Code Coverage**: Automated coverage reports with lcov
- **Static Analysis**: clang-tidy checks
- **Benchmarks**: Performance regression testing

## Contributing

We welcome contributions! Areas of interest:

- Additional shape primitives (Path, Polygon, Arc)
- GPU acceleration (OpenGL/Vulkan compute)
- More SIMD optimizations
- Python API completeness
- Performance improvements

Please ensure:
- All tests pass (`make check`)
- Code is formatted (`make format`)
- New code has tests
- Benchmarks for performance-critical code

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.