# C++ Architecture for Claude Draw

## Overview

This document outlines the architecture of the high-performance C++ backend for the Claude Draw library. The C++ implementation is designed to handle millions of graphics objects efficiently while maintaining full API compatibility with the existing Python library.

### Goals

1. **Performance**: Achieve 50-100x performance improvement over pure Python implementation
2. **Memory Efficiency**: Reduce memory usage from ~2KB to ~40 bytes per object
3. **Scalability**: Handle millions of objects with sub-millisecond operations
4. **Compatibility**: Maintain full API compatibility with existing Python code
5. **Extensibility**: Support future optimizations and features

### Design Principles

- **Zero-Copy Operations**: Minimize data copying between Python and C++
- **Cache-Friendly Layout**: Optimize data structures for CPU cache efficiency
- **SIMD Optimization**: Leverage AVX2/AVX512 for parallel processing
- **Batch Processing**: Design APIs for operating on thousands of objects simultaneously
- **Memory Pooling**: Use custom allocators to minimize allocation overhead

## Core Architecture

### Memory Layout Design

The C++ implementation uses a radically different memory layout optimized for performance:

```cpp
// Python/Pydantic approach (inefficient)
class Circle {
    std::string id;          // 32+ bytes
    Point2D center;          // 16+ bytes (with padding)
    double radius;           // 8 bytes
    Color fill_color;        // 16+ bytes
    Color stroke_color;      // 16+ bytes
    double stroke_width;     // 8 bytes
    Transform2D transform;   // 72+ bytes
    // Total: ~200+ bytes with overhead
};

// C++ optimized approach
struct CircleData {
    float center_x;          // 4 bytes
    float center_y;          // 4 bytes
    float radius;            // 4 bytes
    uint32_t fill_color;     // 4 bytes (packed RGBA)
    uint32_t stroke_color;   // 4 bytes (packed RGBA)
    float stroke_width;      // 4 bytes
    uint16_t flags;          // 2 bytes (visible, filled, stroked)
    uint16_t transform_idx;  // 2 bytes (index into transform pool)
    uint32_t id;            // 4 bytes (index, not UUID)
    // Total: 32 bytes (cache line friendly)
};
```

### Type System

```cpp
namespace claude_draw {

// Core types use fixed-size, cache-aligned structures
struct alignas(8) Point2D {
    float x, y;
    
    // SIMD-friendly operations
    Point2D operator+(const Point2D& other) const;
    float distance_squared(const Point2D& other) const;
};

// Color packed for efficiency
struct Color {
    union {
        uint32_t rgba;
        struct { uint8_t r, g, b, a; };
    };
    
    static Color from_floats(float r, float g, float b, float a = 1.0f);
    static Color blend(Color src, Color dst, float alpha);
};

// Transform stored separately for objects that need it
struct alignas(32) Transform2D {
    float m[6];  // 2x3 affine matrix
    
    Point2D transform_point(const Point2D& p) const;
    Transform2D compose(const Transform2D& other) const;
};

// Bounding box optimized for SIMD
struct alignas(16) BoundingBox {
    float min_x, min_y, max_x, max_y;
    
    bool intersects(const BoundingBox& other) const;
    BoundingBox merge(const BoundingBox& other) const;
};

} // namespace claude_draw
```

## Component Design

### Shape Storage

Shapes are stored using Structure of Arrays (SoA) for SIMD efficiency:

```cpp
template<typename ShapeData>
class ShapeStorage {
private:
    // Structure of Arrays for SIMD processing
    std::vector<float, AlignedAllocator<float, 32>> x_coords;
    std::vector<float, AlignedAllocator<float, 32>> y_coords;
    std::vector<float, AlignedAllocator<float, 32>> radii;  // for circles
    std::vector<uint32_t> colors;
    std::vector<uint16_t> flags;
    std::vector<uint32_t> ids;
    
    // Optional transform pool (most shapes don't need transforms)
    TransformPool transform_pool;
    
public:
    // Batch operations
    void transform_batch(size_t start, size_t count, const Transform2D& t);
    void set_color_batch(size_t start, size_t count, Color color);
    std::vector<uint32_t> query_region(const BoundingBox& region) const;
};
```

### Container Architecture

```cpp
class Container {
private:
    // Spatial index for fast queries
    RTree<uint32_t, float, 2> spatial_index;
    
    // Separate storage for different shape types
    ShapeStorage<CircleData> circles;
    ShapeStorage<RectangleData> rectangles;
    ShapeStorage<EllipseData> ellipses;
    ShapeStorage<LineData> lines;
    
    // Hierarchical structure
    std::vector<uint32_t> child_containers;
    
    // Cached aggregate bounds
    mutable std::optional<BoundingBox> cached_bounds;
    
public:
    // Batch shape creation
    std::span<uint32_t> add_circles(size_t count, const CircleSpec* specs);
    
    // Parallel visitor traversal
    template<typename Visitor>
    void accept_parallel(Visitor& visitor, size_t thread_count = 0);
    
    // Spatial queries
    std::vector<uint32_t> query_viewport(const BoundingBox& viewport);
    std::optional<uint32_t> hit_test(const Point2D& point);
};
```

### Memory Management

```cpp
// Arena allocator for shapes
template<size_t BlockSize = 1024 * 1024>  // 1MB blocks
class ArenaAllocator {
private:
    struct Block {
        std::unique_ptr<uint8_t[]> memory;
        size_t used;
    };
    
    std::vector<Block> blocks;
    std::vector<void*> free_list;
    
public:
    template<typename T>
    T* allocate() {
        if constexpr (sizeof(T) <= 64) {
            // Use free list for small objects
            if (!free_list.empty()) {
                void* ptr = free_list.back();
                free_list.pop_back();
                return new(ptr) T();
            }
        }
        // Allocate from current block
        return allocate_from_block<T>();
    }
    
    template<typename T>
    void deallocate(T* ptr) {
        ptr->~T();
        if constexpr (sizeof(T) <= 64) {
            free_list.push_back(ptr);
        }
        // Large objects: do nothing (arena cleanup)
    }
};

// Object ID management
class IDManager {
private:
    std::vector<uint32_t> free_ids;
    uint32_t next_id = 1;
    
public:
    uint32_t allocate_id() {
        if (!free_ids.empty()) {
            uint32_t id = free_ids.back();
            free_ids.pop_back();
            return id;
        }
        return next_id++;
    }
    
    void release_id(uint32_t id) {
        free_ids.push_back(id);
    }
};
```

## Performance Optimizations

### SIMD Operations

```cpp
// AVX2 implementation for transforming points
void transform_points_avx2(
    const float* x_in, const float* y_in,
    float* x_out, float* y_out,
    size_t count, const Transform2D& transform
) {
    const __m256 m00 = _mm256_set1_ps(transform.m[0]);
    const __m256 m01 = _mm256_set1_ps(transform.m[1]);
    const __m256 m10 = _mm256_set1_ps(transform.m[2]);
    const __m256 m11 = _mm256_set1_ps(transform.m[3]);
    const __m256 m02 = _mm256_set1_ps(transform.m[4]);
    const __m256 m12 = _mm256_set1_ps(transform.m[5]);
    
    size_t i = 0;
    for (; i + 8 <= count; i += 8) {
        __m256 x = _mm256_load_ps(&x_in[i]);
        __m256 y = _mm256_load_ps(&y_in[i]);
        
        __m256 new_x = _mm256_fmadd_ps(x, m00, _mm256_fmadd_ps(y, m01, m02));
        __m256 new_y = _mm256_fmadd_ps(x, m10, _mm256_fmadd_ps(y, m11, m12));
        
        _mm256_store_ps(&x_out[i], new_x);
        _mm256_store_ps(&y_out[i], new_y);
    }
    
    // Handle remaining elements
    for (; i < count; ++i) {
        x_out[i] = x_in[i] * transform.m[0] + y_in[i] * transform.m[1] + transform.m[4];
        y_out[i] = x_in[i] * transform.m[2] + y_in[i] * transform.m[3] + transform.m[5];
    }
}

// CPU dispatch based on runtime detection
struct SimdDispatcher {
    using TransformFunc = void(*)(const float*, const float*, float*, float*, size_t, const Transform2D&);
    
    static TransformFunc get_transform_func() {
        if (cpu_supports_avx512()) return transform_points_avx512;
        if (cpu_supports_avx2()) return transform_points_avx2;
        return transform_points_scalar;
    }
};
```

### Batch APIs

```cpp
// Batch shape creation for maximum efficiency
class BatchBuilder {
private:
    std::vector<CircleSpec> pending_circles;
    std::vector<RectangleSpec> pending_rectangles;
    Container* target;
    
public:
    BatchBuilder& add_circle(float x, float y, float radius) {
        pending_circles.push_back({x, y, radius, current_style});
        return *this;
    }
    
    BatchBuilder& add_rectangle(float x, float y, float w, float h) {
        pending_rectangles.push_back({x, y, w, h, current_style});
        return *this;
    }
    
    std::vector<ShapeHandle> commit() {
        std::vector<ShapeHandle> handles;
        
        if (!pending_circles.empty()) {
            auto ids = target->add_circles(pending_circles.size(), pending_circles.data());
            handles.insert(handles.end(), ids.begin(), ids.end());
            pending_circles.clear();
        }
        
        // Similar for other shapes...
        return handles;
    }
};
```

### Spatial Indexing

```cpp
// R-tree configuration optimized for graphics
using RTreeConfig = rtree::Config<
    16,     // max elements per node
    8,      // min elements per node
    32,     // max elements in leaf
    float,  // coordinate type
    2       // dimensions
>;

class SpatialIndex {
private:
    rtree::RTree<uint32_t, float, 2, RTreeConfig> tree;
    
public:
    void insert(uint32_t id, const BoundingBox& bounds) {
        tree.Insert(
            {bounds.min_x, bounds.min_y},
            {bounds.max_x, bounds.max_y},
            id
        );
    }
    
    std::vector<uint32_t> query_region(const BoundingBox& region) {
        std::vector<uint32_t> results;
        tree.Search(
            {region.min_x, region.min_y},
            {region.max_x, region.max_y},
            [&results](uint32_t id) {
                results.push_back(id);
                return true;  // continue search
            }
        );
        return results;
    }
};
```

## Python Integration

### Binding Architecture

```cpp
namespace py = pybind11;

// Zero-copy numpy integration
py::array_t<float> get_circle_positions(const Container& container) {
    auto& circles = container.get_circles();
    size_t count = circles.size();
    
    // Create numpy array that shares memory with C++ data
    return py::array_t<float>(
        {count, 2},  // shape: (n, 2)
        {2 * sizeof(float), sizeof(float)},  // strides
        circles.get_positions_ptr(),  // data pointer
        py::cast(&container)  // keep container alive
    );
}

// Lazy conversion wrapper
class LazyCircle {
private:
    Container* container;
    uint32_t id;
    mutable std::optional<py::object> py_cache;
    
public:
    py::object to_python() const {
        if (!py_cache) {
            // Convert to Python only when needed
            auto data = container->get_circle_data(id);
            py_cache = create_python_circle(data);
        }
        return *py_cache;
    }
};
```

### API Compatibility Layer

```cpp
// Maintain compatibility with existing Python API
class Circle : public DrawableBase {
public:
    // Python-compatible constructor
    Circle(const Point2D& center, float radius, 
           std::optional<Color> fill = std::nullopt,
           std::optional<Color> stroke = std::nullopt)
    {
        // Delegate to efficient C++ implementation
        handle = get_current_container()->add_circle(
            center.x, center.y, radius,
            fill.value_or(Color::black()),
            stroke.value_or(Color::none())
        );
    }
    
    // Property access (lazy)
    Point2D get_center() const {
        auto [x, y] = get_container()->get_circle_center(handle);
        return Point2D{x, y};
    }
    
    // Immutable updates return new objects
    Circle with_center(const Point2D& new_center) const {
        Circle new_circle = *this;
        get_container()->update_circle_center(new_circle.handle, new_center.x, new_center.y);
        return new_circle;
    }
};
```

## Testing Strategy

### Unit Testing with Google Test

```cpp
// Test fixture for shape tests
class ShapeTest : public ::testing::Test {
protected:
    void SetUp() override {
        container = std::make_unique<Container>();
        allocator = std::make_unique<ArenaAllocator<>>();
    }
    
    void TearDown() override {
        // Cleanup handled by smart pointers
    }
    
    std::unique_ptr<Container> container;
    std::unique_ptr<ArenaAllocator<>> allocator;
};

// Performance test example
TEST_F(ShapeTest, CircleCreationPerformance) {
    const size_t count = 1'000'000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    BatchBuilder builder(container.get());
    for (size_t i = 0; i < count; ++i) {
        builder.add_circle(i * 10.0f, i * 10.0f, 5.0f);
    }
    auto handles = builder.commit();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should create 1M circles in < 100ms (0.1μs per circle)
    EXPECT_LT(duration.count(), 100'000);
    EXPECT_EQ(handles.size(), count);
}

// SIMD correctness test
TEST_F(ShapeTest, SimdTransformCorrectness) {
    const size_t count = 1000;
    std::vector<float> x(count), y(count);
    std::vector<float> x_scalar(count), y_scalar(count);
    std::vector<float> x_simd(count), y_simd(count);
    
    // Initialize with random values
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    
    for (size_t i = 0; i < count; ++i) {
        x[i] = dist(gen);
        y[i] = dist(gen);
    }
    
    Transform2D transform = Transform2D::rotate(M_PI / 4).translate(100, 200);
    
    // Scalar implementation
    transform_points_scalar(x.data(), y.data(), x_scalar.data(), y_scalar.data(), count, transform);
    
    // SIMD implementation
    auto simd_func = SimdDispatcher::get_transform_func();
    simd_func(x.data(), y.data(), x_simd.data(), y_simd.data(), count, transform);
    
    // Compare results (within floating point tolerance)
    for (size_t i = 0; i < count; ++i) {
        EXPECT_NEAR(x_scalar[i], x_simd[i], 1e-5f);
        EXPECT_NEAR(y_scalar[i], y_simd[i], 1e-5f);
    }
}
```

### Benchmarking

```cpp
// Google Benchmark integration
#include <benchmark/benchmark.h>

static void BM_CircleCreation(benchmark::State& state) {
    Container container;
    const size_t batch_size = state.range(0);
    
    for (auto _ : state) {
        BatchBuilder builder(&container);
        for (size_t i = 0; i < batch_size; ++i) {
            builder.add_circle(i * 10.0f, i * 10.0f, 5.0f);
        }
        auto handles = builder.commit();
        benchmark::DoNotOptimize(handles);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK(BM_CircleCreation)->Range(1, 1<<20);

static void BM_SpatialQuery(benchmark::State& state) {
    Container container;
    const size_t num_shapes = 1'000'000;
    
    // Setup: create shapes
    BatchBuilder builder(&container);
    for (size_t i = 0; i < num_shapes; ++i) {
        float x = (i % 1000) * 10.0f;
        float y = (i / 1000) * 10.0f;
        builder.add_circle(x, y, 5.0f);
    }
    builder.commit();
    
    BoundingBox query_region{400, 400, 600, 600};
    
    for (auto _ : state) {
        auto results = container.query_viewport(query_region);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetLabel("1M shapes");
}

BENCHMARK(BM_SpatialQuery);
```

## Build System

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.20)
project(claude_draw_cpp VERSION 1.0.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# CPU feature detection
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)

# Main library
add_library(claude_draw_core STATIC
    src/core/point2d.cpp
    src/core/color.cpp
    src/core/transform2d.cpp
    src/core/bounding_box.cpp
    src/shapes/circle.cpp
    src/shapes/rectangle.cpp
    src/containers/container.cpp
    src/memory/arena_allocator.cpp
    src/simd/transform_avx2.cpp
)

target_compile_options(claude_draw_core PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CONFIG:Debug>:-g -fsanitize=address>
)

if(COMPILER_SUPPORTS_AVX2)
    target_compile_definitions(claude_draw_core PRIVATE HAS_AVX2)
    set_source_files_properties(src/simd/transform_avx2.cpp 
        PROPERTIES COMPILE_FLAGS "-mavx2")
endif()

# Python module
pybind11_add_module(_claude_draw_cpp
    src/bindings/module.cpp
    src/bindings/shapes.cpp
    src/bindings/containers.cpp
)

target_link_libraries(_claude_draw_cpp PRIVATE claude_draw_core)

# Tests
enable_testing()

add_executable(claude_draw_tests
    tests/test_main.cpp
    tests/test_shapes.cpp
    tests/test_containers.cpp
    tests/test_simd.cpp
)

target_link_libraries(claude_draw_tests 
    claude_draw_core 
    GTest::gtest 
    GTest::gtest_main
)

# Benchmarks
add_executable(claude_draw_benchmarks
    benchmarks/bench_main.cpp
    benchmarks/bench_shapes.cpp
    benchmarks/bench_spatial.cpp
)

target_link_libraries(claude_draw_benchmarks
    claude_draw_core
    benchmark::benchmark
)

# Installation
install(TARGETS _claude_draw_cpp
    LIBRARY DESTINATION ${Python_SITEARCH}/claude_draw
)
```

## Performance Targets

### Object Creation
- **Current**: ~24μs per object
- **Target**: <0.5μs per object
- **Method**: Batch APIs, arena allocation, no validation overhead

### Memory Usage
- **Current**: ~2KB per object
- **Target**: ~40 bytes per object
- **Method**: Packed structures, shared transform pool, ID instead of UUID

### Batch Operations
- **Target**: Transform 1M objects in <10ms
- **Method**: SIMD operations, parallel execution

### Spatial Queries
- **Target**: Query 10K objects from 1M in <1ms
- **Method**: R-tree spatial index

### Python Integration
- **Target**: <1μs overhead per Python API call
- **Method**: Lazy conversion, zero-copy numpy arrays

## Future Optimizations

1. **GPU Acceleration**: OpenGL/Vulkan compute shaders for massive parallelism
2. **Custom Memory Pages**: OS-level optimizations for huge datasets
3. **Incremental Computation**: Dirty flags and incremental updates
4. **Hardware Acceleration**: Intel oneAPI integration
5. **Distributed Processing**: Multi-machine rendering for billions of objects

## Conclusion

This architecture provides a solid foundation for achieving the 50-100x performance improvement required for handling millions of graphics objects. The combination of efficient memory layout, SIMD optimization, spatial indexing, and smart Python integration ensures both high performance and ease of use.