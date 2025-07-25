# Makefile Usage Guide

The C++ component of Claude Draw includes a comprehensive Makefile that simplifies common development tasks. This guide explains how to use it effectively.

## Quick Start

```bash
# Build the project (Release mode)
make

# Build in Debug mode
make debug

# Run tests
make test

# Run all checks (tests + lint + format)
make check
```

## Common Development Workflow

### 1. Initial Setup

```bash
# Generate compile_commands.json for your IDE
make compile-commands

# Build in debug mode for development
make debug
```

### 2. Development Cycle

```bash
# Quick rebuild (uses existing Debug build)
make quick

# Run tests after changes
make test

# Or use watch mode (requires 'entr' or 'inotifywait')
make watch
```

### 3. Before Committing

```bash
# Run all checks
make check

# Or individually:
make test         # Run unit tests
make lint         # Static analysis with clang-tidy
make format-check # Check code formatting

# Fix formatting issues
make format
```

## Build Targets

### Basic Builds

| Target | Description | Equivalent CMake Command |
|--------|-------------|-------------------------|
| `make` or `make all` | Build in Release mode | `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` |
| `make debug` | Build in Debug mode | `cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug` |
| `make clean` | Remove all build artifacts | `rm -rf build*` |
| `make install` | Install libraries and headers | `cmake --install build-release` |

### Testing

| Target | Description |
|--------|-------------|
| `make test` | Run unit tests |
| `make test-verbose` | Run tests with detailed output |
| `make bench` | Run performance benchmarks |
| `make memcheck` | Run tests with valgrind memory checker |

### Code Quality

| Target | Description |
|--------|-------------|
| `make coverage` | Generate code coverage report |
| `make coverage-html` | Generate HTML coverage report |
| `make sanitize` | Build and test with AddressSanitizer |
| `make lint` | Run clang-tidy static analysis |
| `make format` | Auto-format code with clang-format |
| `make format-check` | Check if code needs formatting |

### Python Bindings

| Target | Description |
|--------|-------------|
| `make python` | Build Python bindings only |
| `make test-python` | Test Python bindings |
| `make install-python` | Install Python module |

## Advanced Usage

### Parallel Builds

```bash
# Use 8 parallel jobs
make JOBS=8

# Auto-detect optimal job count (default)
make
```

### Custom Build Directory

```bash
# Use custom build directory
make BUILD_DIR=my-build
```

### Verbose Output

```bash
# Enable verbose output
make V=1
```

### Combined Operations

```bash
# Build and test in one command
make debug test

# Full validation before commit
make clean all test bench check
```

## CI Integration

The Makefile provides special targets for CI systems:

```bash
# CI-specific targets
make ci-build      # Build for CI
make ci-test       # Run tests for CI
make ci-coverage   # Generate coverage for CI
make ci-sanitize   # Run sanitizers for CI
```

These targets are used in the simplified CI workflow (`.github/workflows/cpp-ci-simplified.yml`).

## Troubleshooting

### Missing Tools

The Makefile will inform you if required tools are missing:

```bash
$ make lint
clang-tidy not found! Install it to use this target.
```

Install missing tools:
- **Ubuntu/Debian**: `sudo apt-get install clang-tidy clang-format lcov valgrind`
- **macOS**: `brew install llvm lcov valgrind`

### Build Errors

If you encounter build errors:

1. Clean and rebuild:
   ```bash
   make clean
   make debug
   ```

2. Check CMake version:
   ```bash
   cmake --version  # Should be >= 3.20
   ```

3. Verify dependencies:
   ```bash
   make info  # Shows configuration
   ```

## Tips and Tricks

### Shortcuts

The Makefile includes short aliases for common commands:

```bash
make b  # Same as 'make all'
make t  # Same as 'make test'
make c  # Same as 'make clean'
make d  # Same as 'make debug'
```

### Development Mode

For active development, use this workflow:

```bash
# Terminal 1: Watch mode
make watch

# Terminal 2: Make changes
vim src/core/point2d.cpp

# Tests run automatically when you save!
```

### Performance Analysis

```bash
# Run benchmarks
make bench

# Profile with perf (Linux only)
make perf

# Check memory usage
make memcheck
```

### IDE Integration

Most IDEs can use the generated `compile_commands.json`:

```bash
# Generate for clangd, VSCode, etc.
make compile-commands
```

## Environment Variables

The Makefile respects these environment variables:

- `BUILD_TYPE`: Debug or Release (default: Release)
- `JOBS`: Number of parallel build jobs (default: auto-detect)
- `CMAKE`: Path to cmake executable
- `PYTHON`: Python interpreter (default: python3)
- `CC`: C compiler
- `CXX`: C++ compiler

Example:
```bash
BUILD_TYPE=Debug JOBS=4 make
```

## Extending the Makefile

To add new targets, edit the Makefile and follow these patterns:

```makefile
# Simple target
.PHONY: my-target
my-target:
	@echo "$(GREEN)Running my target...$(NC)"
	@# Your commands here

# Target with dependencies
.PHONY: my-complex-target
my-complex-target: build test
	@echo "$(GREEN)Running after build and test...$(NC)"
	@# Your commands here
```

Remember to add `.PHONY:` for targets that don't create files.