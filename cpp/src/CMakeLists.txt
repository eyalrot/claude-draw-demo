# Core library
add_library(claude_draw_core STATIC
    # Core files
    core/placeholder.cpp
    core/simd.cpp
    
    # Core types (to be added)
    # core/point2d.cpp
    # core/color.cpp
    # core/transform2d.cpp
    # core/bounding_box.cpp
    
    # Shapes (to be added)
    # shapes/circle.cpp
    # shapes/rectangle.cpp
    # shapes/ellipse.cpp
    # shapes/line.cpp
    
    # Containers (to be added)
    # containers/container.cpp
    # containers/spatial_index.cpp
    
    # Memory management (to be added)
    # memory/arena_allocator.cpp
    # memory/object_pool.cpp
    
    # SIMD implementations (to be added)
    # simd/transform_scalar.cpp
)

# Include directories
target_include_directories(claude_draw_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Ensure static library can be linked into shared library
set_target_properties(claude_draw_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Compile definitions based on SIMD support
if(COMPILER_SUPPORTS_AVX2)
    target_compile_definitions(claude_draw_core PUBLIC HAS_AVX2)
    
    # Add AVX2 specific files when available
    # target_sources(claude_draw_core PRIVATE simd/transform_avx2.cpp)
    # set_source_files_properties(simd/transform_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
endif()

if(COMPILER_SUPPORTS_AVX512)
    target_compile_definitions(claude_draw_core PUBLIC HAS_AVX512)
    
    # Add AVX512 specific files when available
    # target_sources(claude_draw_core PRIVATE simd/transform_avx512.cpp)
    # set_source_files_properties(simd/transform_avx512.cpp PROPERTIES COMPILE_FLAGS "-mavx512f")
endif()

# Python bindings
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings)
endif()

# Installation
install(TARGETS claude_draw_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)