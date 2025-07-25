#!/bin/bash

# Build script for Debug configuration with sanitizers

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_ROOT/build-debug"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}Building Claude Draw C++ in Debug mode with sanitizers...${NC}"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Configuring...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_SANITIZERS=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=OFF \
    -DBUILD_PYTHON_BINDINGS=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build . -j$(nproc)

echo -e "${GREEN}Build complete! Binaries are in: $BUILD_DIR${NC}"
echo -e "${YELLOW}To run tests with sanitizers: cd $BUILD_DIR && ctest --verbose${NC}"