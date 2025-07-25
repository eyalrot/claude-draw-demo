# Building Claude Draw C++ Optimization Layer

This document describes how to build the C++ optimization layer for Claude Draw.

## Prerequisites

### Required Tools
- CMake 3.20 or higher
- C++20 compatible compiler:
  - GCC 10+ (Linux)
  - Clang 12+ (macOS/Linux)
  - MSVC 2019+ (Windows)
- Python 3.8+ with development headers
- Git

### Optional Tools
- Ninja (faster builds)
- lcov (code coverage on Linux/macOS)
- clang-tidy (static analysis)
- ccache (build caching)

### Installing Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    python3-dev \
    lcov \
    clang-tidy \
    ccache
```

#### macOS
```bash
brew install cmake ninja python lcov ccache
```

#### Windows
Install Visual Studio 2019 or later with C++ development tools.
Install Python from python.org.
```powershell
choco install cmake ninja
```

## Building

### Quick Build
```bash
cd cpp
mkdir build && cd build
cmake .. -GNinja
ninja
```

### Build Configurations

#### Release Build (Optimized)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

#### Debug Build (with debug symbols)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

#### Debug Build with Sanitizers
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
```

#### Coverage Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | ON | Build unit tests |
| `BUILD_BENCHMARKS` | ON | Build performance benchmarks |
| `BUILD_PYTHON_BINDINGS` | ON | Build Python bindings |
| `ENABLE_COVERAGE` | OFF | Enable code coverage |
| `ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer and UBSanitizer |

### Using CMake Presets

The project includes CMake presets for common configurations:

```bash
# List available presets
cmake --list-presets

# Build with a preset
cmake --preset=release
cmake --build --preset=release

# Run tests with a preset
ctest --preset=release
```

Available presets:
- `debug` - Debug build with sanitizers
- `release` - Release build with optimizations
- `coverage` - Debug build with coverage
- `dev` - Development build with all features

### Build Scripts

Convenience scripts are provided in the `scripts/` directory:

```bash
# Debug build
./scripts/build-debug.sh

# Release build
./scripts/build-release.sh

# Coverage build and report
./scripts/coverage.sh
```

## Testing

### Running Tests
```bash
# In build directory
ctest

# Verbose output
ctest --verbose

# Run specific test
ctest -R SimdCapabilities

# Run with output on failure
ctest --output-on-failure
```

### Custom Test Targets
```bash
# Run tests with verbose output
make test_verbose

# Run tests with memory checking (requires valgrind)
make test_memcheck
```

## Benchmarks

### Running Benchmarks
```bash
# Run all benchmarks
./benchmarks/claude_draw_benchmarks

# Run specific benchmark
./benchmarks/claude_draw_benchmarks --benchmark_filter=BM_AlignedAlloc

# Quick benchmarks (reduced time)
make run-benchmarks-quick

# Generate JSON output
make run-benchmarks-json
```

### Benchmark Options
- `--benchmark_min_time=0.1s` - Minimum time per benchmark
- `--benchmark_repetitions=5` - Number of repetitions
- `--benchmark_format=json` - Output format
- `--benchmark_out=results.json` - Output file

## Code Coverage

### Generate Coverage Report
```bash
# Build with coverage
cmake .. -DENABLE_COVERAGE=ON
make

# Run tests
make test

# Generate report
make coverage-html

# View report
open coverage_html/index.html
```

### Using Coverage Script
```bash
./scripts/coverage.sh
# Report will be in cpp/coverage/index.html
```

## Static Analysis

### Running clang-tidy
```bash
# Configure with compile commands
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run on all files
find src include -name '*.cpp' -o -name '*.h' | \
    xargs clang-tidy -p build

# Run with fixes
clang-tidy -p build -fix src/core/simd.cpp
```

## Python Bindings

### Building Python Module
The Python module is built automatically when `BUILD_PYTHON_BINDINGS=ON`.

### Testing Python Bindings
```bash
cd cpp
python3 test_bindings.py
```

### Installing Python Module
```bash
# In build directory
make install
```

## Troubleshooting

### Common Issues

1. **CMake can't find Python**
   ```bash
   cmake .. -DPython3_ROOT_DIR=/path/to/python
   ```

2. **Missing dependencies**
   - Make sure all prerequisites are installed
   - Check CMake output for missing packages

3. **Compiler errors**
   - Ensure your compiler supports C++20
   - Update to the latest compiler version

4. **Linking errors with Python**
   - Install python-dev or python3-dev package
   - Ensure Python and compiler architectures match

### Platform-Specific Notes

#### Linux
- Use system package manager for dependencies
- May need to install separate -dev packages

#### macOS
- Use Homebrew for dependencies
- Xcode Command Line Tools required

#### Windows
- Use Visual Studio 2019 or later
- Run from Developer Command Prompt
- Python from python.org recommended

## Performance Tuning

### Compiler Optimizations
The release build enables:
- `-O3` optimization level
- `-march=native` for CPU-specific optimizations
- Link-time optimization (LTO)

### SIMD Support
The build system automatically detects:
- SSE2, SSE3, SSSE3, SSE4.1, SSE4.2
- AVX, AVX2, AVX512
- FMA instructions

Check detected features:
```bash
./build/benchmarks/claude_draw_benchmarks | head -20
```

## Contributing

When contributing to the C++ code:
1. Run tests: `ctest`
2. Run benchmarks to check performance impact
3. Ensure code passes clang-tidy checks
4. Maintain >80% code coverage
5. Follow the coding style (use clang-format)