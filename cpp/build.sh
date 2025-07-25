#!/bin/bash

# Build script for Claude Draw C++ implementation

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CLEAN=0
TESTS=1
BENCHMARKS=1
COVERAGE=0
SANITIZERS=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        --no-tests)
            TESTS=0
            shift
            ;;
        --no-benchmarks)
            BENCHMARKS=0
            shift
            ;;
        --coverage)
            COVERAGE=1
            BUILD_TYPE="Debug"
            shift
            ;;
        --sanitizers)
            SANITIZERS=1
            BUILD_TYPE="Debug"
            shift
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug          Build in Debug mode (default: Release)"
            echo "  --clean          Clean build directory before building"
            echo "  --no-tests       Don't build tests"
            echo "  --no-benchmarks  Don't build benchmarks"
            echo "  --coverage       Enable code coverage (implies --debug)"
            echo "  --sanitizers     Enable AddressSanitizer and UBSanitizer (implies --debug)"
            echo "  --jobs N         Number of parallel jobs (default: auto-detect)"
            echo "  --help           Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ $CLEAN -eq 1 ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTS=$TESTS \
    -DBUILD_BENCHMARKS=$BENCHMARKS \
    -DENABLE_COVERAGE=$COVERAGE \
    -DENABLE_SANITIZERS=$SANITIZERS

# Build
echo "Building with $JOBS parallel jobs..."
cmake --build . --parallel "$JOBS"

# Run tests if built
if [ $TESTS -eq 1 ] && [ -f tests/claude_draw_tests ]; then
    echo "Running tests..."
    ctest --output-on-failure
fi

echo "Build complete!"