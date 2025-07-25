# Core library
add_library(claude_draw_core STATIC
    # Core files
    core/placeholder.cpp
    core/simd.cpp
    core/point2d.cpp
    core/color.cpp
    core/transform2d.cpp
    
    # Core types
    core/bounding_box.cpp
    
    # Shapes (header-only for now)
    shapes/placeholder.cpp
    
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
    
    # Set AVX2 flags for files that use AVX2 intrinsics
    if(NOT MSVC)
        set_source_files_properties(
            core/point2d.cpp
            core/color.cpp
            core/transform2d.cpp
            core/bounding_box.cpp
            PROPERTIES COMPILE_FLAGS "-mavx2"
        )
    endif()
endif()

if(COMPILER_SUPPORTS_AVX512)
    target_compile_definitions(claude_draw_core PUBLIC HAS_AVX512)
    
    # Set AVX512 flags if needed in the future
    # if(NOT MSVC)
    #     set_source_files_properties(
    #         simd/transform_avx512.cpp
    #         PROPERTIES COMPILE_FLAGS "-mavx512f"
    #     )
    # endif()
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