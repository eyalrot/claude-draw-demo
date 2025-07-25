#!/bin/bash

# Script to generate code coverage reports
# Usage: ./scripts/coverage.sh [clean]

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_ROOT/build-coverage"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if lcov is installed
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov is not installed. Please install it:${NC}"
    echo "  Ubuntu/Debian: sudo apt-get install lcov"
    echo "  macOS: brew install lcov"
    exit 1
fi

# Check if genhtml is installed
if ! command -v genhtml &> /dev/null; then
    echo -e "${RED}Error: genhtml is not installed (part of lcov package)${NC}"
    exit 1
fi

# Clean if requested
if [ "$1" == "clean" ]; then
    echo -e "${YELLOW}Cleaning coverage data...${NC}"
    rm -rf "$BUILD_DIR"
    rm -rf "$PROJECT_ROOT/coverage"
    exit 0
fi

# Create build directory
echo -e "${GREEN}Creating coverage build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with coverage enabled
echo -e "${GREEN}Configuring build with coverage enabled...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DENABLE_COVERAGE=ON \
         -DBUILD_TESTS=ON \
         -DBUILD_BENCHMARKS=OFF \
         -DBUILD_PYTHON_BINDINGS=OFF

# Build
echo -e "${GREEN}Building project...${NC}"
make -j$(nproc)

# Run tests
echo -e "${GREEN}Running tests...${NC}"
ctest --verbose

# Capture coverage data
echo -e "${GREEN}Capturing coverage data...${NC}"
lcov --capture \
     --directory . \
     --output-file coverage.info \
     --no-external \
     --exclude '*/tests/*' \
     --exclude '*/build/*' \
     --exclude '*/_deps/*'

# Generate HTML report
echo -e "${GREEN}Generating HTML report...${NC}"
genhtml coverage.info \
        --output-directory "$PROJECT_ROOT/coverage" \
        --demangle-cpp \
        --num-spaces 4 \
        --sort \
        --title "Claude Draw C++ Coverage Report" \
        --function-coverage \
        --branch-coverage \
        --legend

# Print summary
echo -e "${GREEN}Coverage report generated!${NC}"
echo -e "Open ${YELLOW}$PROJECT_ROOT/coverage/index.html${NC} in your browser to view the report."

# Show coverage summary
echo -e "\n${GREEN}Coverage Summary:${NC}"
lcov --summary coverage.info