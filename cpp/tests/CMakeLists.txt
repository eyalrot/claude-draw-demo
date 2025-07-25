# Test executable
add_executable(claude_draw_tests
    # Main test file
    test_main.cpp
    
    # Core tests
    core/test_simd.cpp
    core/test_point2d.cpp
    core/test_color.cpp
    
    # Core tests (to be added)
    # core/test_color.cpp
    # core/test_transform2d.cpp
    # core/test_bounding_box.cpp
    
    # Shape tests (to be added)
    # shapes/test_circle.cpp
    # shapes/test_rectangle.cpp
    # shapes/test_ellipse.cpp
    # shapes/test_line.cpp
    
    # Container tests (to be added)
    # containers/test_group.cpp
    # containers/test_layer.cpp
    # containers/test_spatial_index.cpp
)

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