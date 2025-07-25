# Benchmark executable
add_executable(claude_draw_benchmarks
    # Main benchmark file
    bench_main.cpp
    
    # Core type benchmarks
    bench_core_types.cpp
    
    # SIMD benchmarks
    bench_simd.cpp
    
    # Shape benchmarks (to be added)
    # bench_shapes.cpp
    
    # Container benchmarks (to be added)
    # bench_containers.cpp
    
    # Memory benchmarks (to be added)
    # bench_memory.cpp
)

# Link with core library and Google Benchmark
target_link_libraries(claude_draw_benchmarks 
    PRIVATE 
        claude_draw_core
        benchmark::benchmark
)

# Include directories
target_include_directories(claude_draw_benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

# Add custom target to run benchmarks
add_custom_target(run-benchmarks
    COMMAND claude_draw_benchmarks
    DEPENDS claude_draw_benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running performance benchmarks"
)

# Add custom target to run benchmarks with JSON output
add_custom_target(run-benchmarks-json
    COMMAND claude_draw_benchmarks --benchmark_format=json --benchmark_out=benchmark_results.json
    DEPENDS claude_draw_benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running benchmarks with JSON output"
)

# Add custom target for quick benchmarks (fewer iterations)
add_custom_target(run-benchmarks-quick
    COMMAND claude_draw_benchmarks --benchmark_min_time=0.1
    DEPENDS claude_draw_benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running quick benchmarks"
)