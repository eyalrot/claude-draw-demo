# Claude Draw C++ Makefile
# This Makefile provides convenient targets for common development tasks
# It wraps the CMake build system for ease of use

# Default configuration
BUILD_TYPE ?= Release
BUILD_DIR ?= build
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CMAKE ?= cmake
CTEST ?= ctest
CMAKE_GENERATOR ?= $(shell which ninja >/dev/null 2>&1 && echo "Ninja" || echo "Unix Makefiles")

# Build directories for different configurations
BUILD_DIR_DEBUG := build-debug
BUILD_DIR_RELEASE := build-release
BUILD_DIR_COVERAGE := build-coverage
BUILD_DIR_SANITIZE := build-sanitize

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	PLATFORM := Linux
endif
ifeq ($(UNAME_S),Darwin)
	PLATFORM := macOS
endif

# Python executable
PYTHON ?= python3

# Default target
.DEFAULT_GOAL := all

# Help target - displays all available targets
.PHONY: help
help:
	@echo "$(BLUE)Claude Draw C++ Build System$(NC)"
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "$(GREEN)Main Targets:$(NC)"
	@echo "  all              Build in Release mode (default)"
	@echo "  debug            Build in Debug mode"
	@echo "  clean            Remove all build directories"
	@echo "  install          Install libraries and headers"
	@echo "  test             Run unit tests"
	@echo "  bench            Run benchmarks"
	@echo ""
	@echo "$(GREEN)Development Targets:$(NC)"
	@echo "  coverage         Generate code coverage report"
	@echo "  coverage-html    Generate HTML coverage report"
	@echo "  sanitize         Build with AddressSanitizer and UBSanitizer"
	@echo "  lint             Run clang-tidy static analysis"
	@echo "  format           Format code with clang-format"
	@echo "  format-check     Check code formatting"
	@echo "  check            Run tests + lint + format check"
	@echo ""
	@echo "$(GREEN)Python Targets:$(NC)"
	@echo "  python           Build Python bindings"
	@echo "  test-python      Test Python bindings"
	@echo "  install-python   Install Python module"
	@echo ""
	@echo "$(GREEN)Advanced Targets:$(NC)"
	@echo "  memcheck         Run tests with valgrind"
	@echo "  watch            Rebuild on file changes"
	@echo "  docs             Generate documentation"
	@echo "  perf             Run performance profiling"
	@echo ""
	@echo "$(GREEN)Options:$(NC)"
	@echo "  BUILD_TYPE=Debug/Release  Build configuration (default: Release)"
	@echo "  JOBS=N                    Parallel build jobs (default: auto)"
	@echo "  V=1                       Verbose output"
	@echo ""
	@echo "$(GREEN)Examples:$(NC)"
	@echo "  make                      # Build in Release mode"
	@echo "  make debug test           # Build Debug and run tests"
	@echo "  make coverage-html        # Generate coverage report"
	@echo "  make check                # Run all checks"

# Build targets
.PHONY: all
all: release

.PHONY: release
release:
	@echo "$(GREEN)Building in Release mode...$(NC)"
	@mkdir -p $(BUILD_DIR_RELEASE)
	@cd $(BUILD_DIR_RELEASE) && $(CMAKE) .. \
		-G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_TESTS=ON \
		-DBUILD_BENCHMARKS=ON \
		-DBUILD_PYTHON_BINDINGS=ON
	@$(CMAKE) --build $(BUILD_DIR_RELEASE) --parallel $(JOBS)
	@echo "$(GREEN)Build complete!$(NC)"

.PHONY: debug
debug:
	@echo "$(GREEN)Building in Debug mode...$(NC)"
	@mkdir -p $(BUILD_DIR_DEBUG)
	@cd $(BUILD_DIR_DEBUG) && $(CMAKE) .. \
		-G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DBUILD_TESTS=ON \
		-DBUILD_BENCHMARKS=ON \
		-DBUILD_PYTHON_BINDINGS=ON
	@$(CMAKE) --build $(BUILD_DIR_DEBUG) --parallel $(JOBS)
	@echo "$(GREEN)Debug build complete!$(NC)"

.PHONY: clean
clean:
	@echo "$(YELLOW)Cleaning build directories...$(NC)"
	@rm -rf build build-* coverage
	@find . -name "*.gcda" -delete 2>/dev/null || true
	@find . -name "*.gcno" -delete 2>/dev/null || true
	@echo "$(GREEN)Clean complete!$(NC)"

.PHONY: install
install: release
	@echo "$(GREEN)Installing...$(NC)"
	@$(CMAKE) --install $(BUILD_DIR_RELEASE)

# Test targets
.PHONY: test
test:
	@if [ -d "$(BUILD_DIR_RELEASE)" ] && [ -f "$(BUILD_DIR_RELEASE)/tests/claude_draw_tests" ]; then \
		echo "$(GREEN)Running tests (Release)...$(NC)"; \
		cd $(BUILD_DIR_RELEASE) && $(CTEST) --output-on-failure; \
	elif [ -d "$(BUILD_DIR_DEBUG)" ] && [ -f "$(BUILD_DIR_DEBUG)/tests/claude_draw_tests" ]; then \
		echo "$(GREEN)Running tests (Debug)...$(NC)"; \
		cd $(BUILD_DIR_DEBUG) && $(CTEST) --output-on-failure; \
	elif [ -d "$(BUILD_DIR)" ] && [ -f "$(BUILD_DIR)/tests/claude_draw_tests" ]; then \
		echo "$(GREEN)Running tests (from existing build)...$(NC)"; \
		cd $(BUILD_DIR) && $(CTEST) --output-on-failure; \
	else \
		echo "$(RED)No test binary found. Run 'make' or 'make debug' first.$(NC)"; \
		exit 1; \
	fi

.PHONY: test-verbose
test-verbose:
	@if [ -d "$(BUILD_DIR_RELEASE)" ] && [ -f "$(BUILD_DIR_RELEASE)/tests/claude_draw_tests" ]; then \
		echo "$(GREEN)Running tests with verbose output...$(NC)"; \
		cd $(BUILD_DIR_RELEASE) && $(CTEST) --verbose; \
	elif [ -d "$(BUILD_DIR)" ] && [ -f "$(BUILD_DIR)/tests/claude_draw_tests" ]; then \
		echo "$(GREEN)Running tests with verbose output (from existing build)...$(NC)"; \
		cd $(BUILD_DIR) && $(CTEST) --verbose; \
	else \
		echo "$(RED)No test binary found. Run 'make' first.$(NC)"; \
		exit 1; \
	fi

.PHONY: bench
bench: release
	@echo "$(GREEN)Running benchmarks...$(NC)"
	@if [ -f "$(BUILD_DIR_RELEASE)/benchmarks/claude_draw_benchmarks" ]; then \
		$(BUILD_DIR_RELEASE)/benchmarks/claude_draw_benchmarks; \
	else \
		echo "$(RED)Benchmark binary not found!$(NC)"; \
		exit 1; \
	fi

# Coverage targets
.PHONY: coverage
coverage:
	@echo "$(GREEN)Building with coverage enabled...$(NC)"
	@mkdir -p $(BUILD_DIR_COVERAGE)
	@cd $(BUILD_DIR_COVERAGE) && $(CMAKE) .. \
		-G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_COVERAGE=ON \
		-DBUILD_TESTS=ON \
		-DBUILD_BENCHMARKS=OFF \
		-DBUILD_PYTHON_BINDINGS=OFF
	@$(CMAKE) --build $(BUILD_DIR_COVERAGE) --parallel $(JOBS)
	@echo "$(GREEN)Running tests for coverage...$(NC)"
	@cd $(BUILD_DIR_COVERAGE) && $(CTEST) --quiet
	@echo "$(GREEN)Generating coverage report...$(NC)"
	@cd $(BUILD_DIR_COVERAGE) && lcov --capture --directory . --output-file coverage.info --no-external
	@cd $(BUILD_DIR_COVERAGE) && lcov --remove coverage.info '*/tests/*' '*/_deps/*' --output-file coverage.info --ignore-errors empty,unused
	@cd $(BUILD_DIR_COVERAGE) && lcov --list coverage.info

.PHONY: coverage-html
coverage-html: coverage
	@echo "$(GREEN)Generating HTML coverage report...$(NC)"
	@cd $(BUILD_DIR_COVERAGE) && genhtml coverage.info --output-directory ../coverage --demangle-cpp
	@echo "$(GREEN)Coverage report generated!$(NC)"
	@echo "Open $(YELLOW)coverage/index.html$(NC) in your browser"

# Sanitizer targets
.PHONY: sanitize
sanitize:
	@echo "$(GREEN)Building with sanitizers enabled...$(NC)"
	@mkdir -p $(BUILD_DIR_SANITIZE)
	@cd $(BUILD_DIR_SANITIZE) && $(CMAKE) .. \
		-G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_SANITIZERS=ON \
		-DBUILD_TESTS=ON \
		-DBUILD_BENCHMARKS=OFF \
		-DBUILD_PYTHON_BINDINGS=OFF
	@$(CMAKE) --build $(BUILD_DIR_SANITIZE) --parallel $(JOBS)
	@echo "$(GREEN)Running tests with sanitizers...$(NC)"
	@cd $(BUILD_DIR_SANITIZE) && $(CTEST) --output-on-failure

# Static analysis
.PHONY: lint
lint:
	@echo "$(GREEN)Running clang-tidy...$(NC)"
	@if command -v clang-tidy >/dev/null 2>&1; then \
		mkdir -p build-lint && \
		cd build-lint && $(CMAKE) .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
		find ../src ../include -name '*.cpp' -o -name '*.h' | \
			xargs clang-tidy -p . --quiet; \
	else \
		echo "$(RED)clang-tidy not found! Install it to use this target.$(NC)"; \
		exit 1; \
	fi

# Code formatting
.PHONY: format
format:
	@echo "$(GREEN)Formatting code...$(NC)"
	@if command -v clang-format >/dev/null 2>&1; then \
		find src include tests benchmarks -name '*.cpp' -o -name '*.h' | \
			xargs clang-format -i; \
		echo "$(GREEN)Formatting complete!$(NC)"; \
	else \
		echo "$(RED)clang-format not found! Install it to use this target.$(NC)"; \
		exit 1; \
	fi

.PHONY: format-check
format-check:
	@echo "$(GREEN)Checking code formatting...$(NC)"
	@if command -v clang-format >/dev/null 2>&1; then \
		find src include tests benchmarks -name '*.cpp' -o -name '*.h' | \
			xargs clang-format --dry-run -Werror 2>&1 | \
			grep -q "error" && \
			(echo "$(RED)Code formatting issues found! Run 'make format' to fix.$(NC)"; exit 1) || \
			echo "$(GREEN)Code formatting is correct!$(NC)"; \
	else \
		echo "$(RED)clang-format not found! Install it to use this target.$(NC)"; \
		exit 1; \
	fi

# Python targets
.PHONY: python
python:
	@echo "$(GREEN)Building Python bindings...$(NC)"
	@if [ -d "$(BUILD_DIR_RELEASE)" ]; then \
		$(CMAKE) --build $(BUILD_DIR_RELEASE) --target _claude_draw_cpp --parallel $(JOBS); \
	else \
		$(MAKE) release; \
	fi

.PHONY: test-python
test-python: python
	@echo "$(GREEN)Testing Python bindings...$(NC)"
	@cd $(BUILD_DIR_RELEASE) && $(PYTHON) ../test_bindings.py

.PHONY: install-python
install-python: python
	@echo "$(GREEN)Installing Python module...$(NC)"
	@cd $(BUILD_DIR_RELEASE) && $(CMAKE) --install . --component python

# Advanced targets
.PHONY: memcheck
memcheck: debug
	@echo "$(GREEN)Running tests with valgrind...$(NC)"
	@if command -v valgrind >/dev/null 2>&1; then \
		cd $(BUILD_DIR_DEBUG) && \
		valgrind --leak-check=full --track-origins=yes --error-exitcode=1 \
			./tests/claude_draw_tests; \
	else \
		echo "$(RED)valgrind not found! Install it to use this target.$(NC)"; \
		exit 1; \
	fi

.PHONY: watch
watch:
	@echo "$(GREEN)Watching for file changes...$(NC)"
	@if command -v entr >/dev/null 2>&1; then \
		find src include tests -name '*.cpp' -o -name '*.h' | \
			entr -c $(MAKE) test; \
	elif command -v inotifywait >/dev/null 2>&1; then \
		while true; do \
			inotifywait -r -e modify,create,delete src include tests; \
			$(MAKE) test; \
		done; \
	else \
		echo "$(RED)Neither 'entr' nor 'inotifywait' found!$(NC)"; \
		echo "Install one of them to use file watching."; \
		exit 1; \
	fi

.PHONY: docs
docs:
	@echo "$(GREEN)Generating documentation...$(NC)"
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile || echo "$(YELLOW)No Doxyfile found. Create one with 'doxygen -g'$(NC)"; \
	else \
		echo "$(RED)doxygen not found! Install it to generate documentation.$(NC)"; \
		exit 1; \
	fi

.PHONY: perf
perf: release
	@echo "$(GREEN)Running performance profiling...$(NC)"
	@if command -v perf >/dev/null 2>&1; then \
		perf record -g $(BUILD_DIR_RELEASE)/benchmarks/claude_draw_benchmarks --benchmark_min_time=2s; \
		perf report; \
	else \
		echo "$(RED)perf not found! Install linux-tools to use this target.$(NC)"; \
		exit 1; \
	fi

# Combined check target
.PHONY: check
check: release test lint format-check
	@echo "$(GREEN)All checks passed!$(NC)"

# Quick rebuild for development
.PHONY: quick
quick:
	@if [ -d "$(BUILD_DIR_DEBUG)" ]; then \
		echo "$(GREEN)Quick rebuild (Debug)...$(NC)"; \
		$(CMAKE) --build $(BUILD_DIR_DEBUG) --parallel $(JOBS); \
	else \
		$(MAKE) debug; \
	fi

# CI-specific targets for simplifying CI workflow
.PHONY: ci-build
ci-build:
	@mkdir -p build
	@cd build && $(CMAKE) .. \
		-G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DBUILD_TESTS=ON \
		-DBUILD_BENCHMARKS=ON \
		-DBUILD_PYTHON_BINDINGS=ON
	@$(CMAKE) --build build --config $(BUILD_TYPE) --parallel $(JOBS)

.PHONY: ci-test
ci-test:
	@cd build && $(CTEST) -C $(BUILD_TYPE) --verbose --output-on-failure

.PHONY: ci-coverage
ci-coverage:
	@echo "$(GREEN)Building with coverage enabled...$(NC)"
	@mkdir -p $(BUILD_DIR_COVERAGE)
	@cd $(BUILD_DIR_COVERAGE) && $(CMAKE) .. \
		-G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_COVERAGE=ON \
		-DBUILD_TESTS=ON \
		-DBUILD_BENCHMARKS=OFF \
		-DBUILD_PYTHON_BINDINGS=OFF
	@$(CMAKE) --build $(BUILD_DIR_COVERAGE) --parallel $(JOBS)
	@echo "$(GREEN)Running tests for coverage...$(NC)"
	@cd $(BUILD_DIR_COVERAGE) && $(CTEST) --quiet
	@echo "$(GREEN)Generating coverage report...$(NC)"
	@cd $(BUILD_DIR_COVERAGE) && lcov --capture --directory . --output-file coverage.info --no-external
	@cd $(BUILD_DIR_COVERAGE) && lcov --remove coverage.info '*/tests/*' '*/_deps/*' --output-file coverage.info --ignore-errors empty,unused
	@cd $(BUILD_DIR_COVERAGE) && lcov --list coverage.info --ignore-errors empty

.PHONY: ci-sanitize
ci-sanitize: sanitize
	@echo "Sanitizer tests completed for CI"

# Print configuration
.PHONY: info
info:
	@echo "$(BLUE)Build Configuration:$(NC)"
	@echo "  BUILD_TYPE: $(BUILD_TYPE)"
	@echo "  CMAKE_GENERATOR: $(CMAKE_GENERATOR)"
	@echo "  JOBS: $(JOBS)"
	@echo "  PLATFORM: $(PLATFORM)"
	@echo "  CMAKE: $(CMAKE)"
	@echo "  PYTHON: $(PYTHON)"

# Create compile_commands.json for IDEs
.PHONY: compile-commands
compile-commands:
	@echo "$(GREEN)Generating compile_commands.json...$(NC)"
	@mkdir -p build
	@cd build && $(CMAKE) .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@ln -sf build/compile_commands.json . 2>/dev/null || true
	@echo "$(GREEN)compile_commands.json created!$(NC)"

# Shortcuts
.PHONY: b
b: all

.PHONY: t
t: test

.PHONY: c
c: clean

.PHONY: d
d: debug

# Prevent Make from removing intermediate files
.SECONDARY:

# Don't print directory changes
MAKEFLAGS += --no-print-directory