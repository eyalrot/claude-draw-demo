#!/bin/bash

# Build script for Release configuration with optimizations

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_ROOT/build-release"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}Building Claude Draw C++ in Release mode with optimizations...${NC}"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Configuring...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_SANITIZERS=OFF \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_PYTHON_BINDINGS=ON \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build . -j$(nproc)

echo -e "${GREEN}Build complete! Binaries are in: $BUILD_DIR${NC}"
echo -e "${YELLOW}Optimizations enabled: -O3 -march=native${NC}"