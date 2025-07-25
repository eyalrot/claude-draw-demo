cmake_minimum_required(VERSION 3.20)
project(claude_draw_cpp VERSION 1.0.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tools like clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(ENABLE_COVERAGE "Enable test coverage" OFF)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSanitizer" OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS TRUE)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Common flags
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Release flags
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native -DNDEBUG)
    endif()
    
    # Debug flags
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
        
        # Sanitizers
        if(ENABLE_SANITIZERS)
            add_compile_options(-fsanitize=address -fsanitize=undefined)
            add_link_options(-fsanitize=address -fsanitize=undefined)
        endif()
    endif()
    
    # Coverage flags
    if(ENABLE_COVERAGE)
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage -fprofile-arcs -ftest-coverage)
    endif()
elseif(MSVC)
    # MSVC flags
    add_compile_options(/W4 /permissive-)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /DNDEBUG)
    endif()
endif()

# CPU feature detection for SIMD
include(CheckCXXCompilerFlag)
if(NOT PLATFORM_WINDOWS OR NOT MSVC)
    check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
else()
    # MSVC always supports these through intrinsics
    set(COMPILER_SUPPORTS_AVX2 TRUE)
    set(COMPILER_SUPPORTS_AVX512 TRUE)
endif()

# Configure RPATH
if(NOT PLATFORM_WINDOWS)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Dependencies
include(FetchContent)

if(BUILD_PYTHON_BINDINGS)
    # Find Python and pybind11
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    
    # Use FetchContent to get pybind11
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG        v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)
endif()

if(BUILD_TESTS)
    # Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Enable testing
    enable_testing()
    include(GoogleTest)
endif()

if(BUILD_BENCHMARKS)
    # Google Benchmark
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)
endif()

# Add subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Coverage targets
if(ENABLE_COVERAGE AND BUILD_TESTS)
    # Add coverage target
    add_custom_target(coverage
        COMMAND ${CMAKE_CTEST_COMMAND} --quiet
        COMMAND lcov --capture --directory . --output-file coverage.info --no-external
        COMMAND lcov --remove coverage.info '*/tests/*' '*/_deps/*' '*/usr/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS claude_draw_tests
        COMMENT "Running tests and generating coverage report"
    )
    
    # Add coverage-html target
    add_custom_target(coverage-html
        COMMAND genhtml coverage.info --output-directory coverage_html --demangle-cpp
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS coverage
        COMMENT "Generating HTML coverage report"
    )
    
    # Add coverage-clean target
    add_custom_target(coverage-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory coverage_html
        COMMAND ${CMAKE_COMMAND} -E remove coverage.info
        COMMAND find . -name "*.gcda" -delete
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Cleaning coverage data"
    )
endif()

# Status messages
message(STATUS "Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "  Build Python bindings: ${BUILD_PYTHON_BINDINGS}")
message(STATUS "  Enable coverage: ${ENABLE_COVERAGE}")
message(STATUS "  Enable sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "  AVX2 support: ${COMPILER_SUPPORTS_AVX2}")
message(STATUS "  AVX512 support: ${COMPILER_SUPPORTS_AVX512}")