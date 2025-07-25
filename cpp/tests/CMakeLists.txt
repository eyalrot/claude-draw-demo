# Test executable
add_executable(claude_draw_tests
    # Main test file
    test_main.cpp
    
    # Core tests
    core/test_simd.cpp
    core/test_point2d.cpp
    core/test_color.cpp
    core/test_transform2d.cpp
    core/test_bounding_box.cpp
    
    # Shape tests
    shapes/test_shape_layout.cpp
    shapes/test_circle.cpp
    shapes/test_circle_optimized.cpp
    shapes/test_rectangle_optimized.cpp
    shapes/test_ellipse_optimized.cpp
    shapes/test_line_optimized.cpp
    shapes/test_shape_batch_ops.cpp
    shapes/test_simd_bounds.cpp
    shapes/test_shape_validation.cpp
    shapes/test_shape_integration.cpp
    # shapes/test_rectangle.cpp
    # shapes/test_ellipse.cpp
    # shapes/test_line.cpp
    
    # Memory tests
    memory/test_object_pool.cpp
    
    # Batch operation tests
    batch/test_batch_operations.cpp
    
    # Integration tests
    # integration/test_integration.cpp
    
    # Container tests
    containers/test_soa_container.cpp
    containers/test_soa_container_advanced.cpp
    containers/test_spatial_index.cpp
    containers/test_spatial_index_performance.cpp
    containers/test_parallel_visitor.cpp
    containers/test_parallel_visitor_performance.cpp
    containers/test_incremental_bounds.cpp
    containers/test_cow_container.cpp
    containers/test_bulk_operations.cpp
    containers/test_container_stress.cpp
    # containers/test_group.cpp
    # containers/test_layer.cpp
)

# Add bindings tests if Python bindings are enabled
if(BUILD_PYTHON_BINDINGS)
    # Create separate test executable for bindings tests
    add_executable(claude_draw_bindings_tests
        test_main.cpp
        bindings/test_conversions.cpp
    )
    
    target_link_libraries(claude_draw_bindings_tests
        PRIVATE
            claude_draw_core
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
            pybind11::embed  # For embedded Python interpreter
    )
    
    target_include_directories(claude_draw_bindings_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/include
    )
    
    target_include_directories(claude_draw_bindings_tests
        SYSTEM PRIVATE
            ${CMAKE_BINARY_DIR}/_deps/googletest-src/googlemock/include
    )
    
    gtest_discover_tests(claude_draw_bindings_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES TIMEOUT 30
    )
endif()

# Link with core library and Google Test
target_link_libraries(claude_draw_tests 
    PRIVATE 
        claude_draw_core
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

# Include directories
target_include_directories(claude_draw_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

# Discover tests for CTest
gtest_discover_tests(claude_draw_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PROPERTIES TIMEOUT 30
)

# Create test data directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# Custom test targets
add_custom_target(test_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS claude_draw_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests with verbose output"
)

add_custom_target(test_memcheck
    COMMAND ${CMAKE_CTEST_COMMAND} -T memcheck
    DEPENDS claude_draw_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests with memory checking"
)