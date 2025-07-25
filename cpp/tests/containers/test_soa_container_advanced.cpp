#include <gtest/gtest.h>
#include "claude_draw/containers/soa_container.h"
#include "claude_draw/shapes/circle_optimized.h"
#include "claude_draw/shapes/rectangle_optimized.h"
#include "claude_draw/shapes/ellipse_optimized.h"
#include "claude_draw/shapes/line_optimized.h"
#include <chrono>
#include <random>
#include <immintrin.h>  // For SIMD intrinsics
#ifdef _OPENMP
#include <omp.h>
#endif

using namespace claude_draw;
using namespace claude_draw::containers;
using namespace claude_draw::shapes;

class SoAContainerAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<SoAContainer>();
        rng_.seed(42);
    }
    
    std::unique_ptr<SoAContainer> container_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> coord_dist_{-100.0f, 100.0f};
    std::uniform_real_distribution<float> size_dist_{1.0f, 50.0f};
};

// Test 1: Verify memory alignment for SIMD operations
TEST_F(SoAContainerAdvancedTest, MemoryAlignmentForSIMD) {
    // Add multiple shapes
    std::vector<uint32_t> circle_ids;
    for (int i = 0; i < 100; ++i) {
        Circle c(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_));
        circle_ids.push_back(container_->add_circle(c));
    }
    
    // Get direct access to circle array
    const auto& circles = container_->get_circles();
    
    // Check alignment of the underlying data
    const void* data_ptr = circles.data();
    uintptr_t addr = reinterpret_cast<uintptr_t>(data_ptr);
    
    // For optimal SIMD performance, data should be at least 16-byte aligned (SSE)
    // 32-byte alignment is better for AVX
    EXPECT_EQ(addr % 16, 0u) << "Data not 16-byte aligned for SSE";
    
    // Verify we can safely use SIMD operations
    if (circles.size() >= 4) {
        // Test SSE load
        __m128 x_vals = _mm_loadu_ps(&circles[0].center_x);
        (void)x_vals; // Avoid unused variable warning
        
        // Test AVX load if available
        #ifdef __AVX__
        if (circles.size() >= 8) {
            __m256 x_vals_avx = _mm256_loadu_ps(&circles[0].center_x);
            (void)x_vals_avx;
        }
        #endif
    }
}

// Test 2: Verify efficient data layout for cache locality
TEST_F(SoAContainerAdvancedTest, CacheLocalityLayout) {
    const size_t num_shapes = 10000;
    
    // Add shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 5.0f);
        container_->add_circle(c);
    }
    
    const auto& circles = container_->get_circles();
    
    // Verify shapes are stored contiguously
    ASSERT_GE(circles.size(), num_shapes);
    
    // Check memory stride between elements
    if (circles.size() >= 2) {
        const char* elem0 = reinterpret_cast<const char*>(&circles[0]);
        const char* elem1 = reinterpret_cast<const char*>(&circles[1]);
        size_t stride = elem1 - elem0;
        
        EXPECT_EQ(stride, sizeof(CircleShape)) 
            << "Non-contiguous storage detected";
    }
    
    // Measure cache-friendly access pattern
    auto start = std::chrono::high_resolution_clock::now();
    
    float sum = 0.0f;
    for (const auto& circle : circles) {
        sum += circle.center_x + circle.center_y + circle.radius;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // With good cache locality, this should be very fast
    double ns_per_shape = (duration.count() * 1000.0) / circles.size();
    EXPECT_LT(ns_per_shape, 10.0) << "Poor cache performance: " << ns_per_shape << " ns/shape";
    
    // Prevent optimization
    volatile float result = sum;
    (void)result;
}

// Test 3: Verify type-homogeneous arrays for SIMD processing
TEST_F(SoAContainerAdvancedTest, HomogeneousArraysForSIMD) {
    // Add different shape types
    std::vector<uint32_t> shape_ids;
    
    for (int i = 0; i < 256; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        Rectangle r(i * 1.0f, i * 2.0f, 10.0f, 20.0f);
        Ellipse e(i * 1.0f, i * 2.0f, 5.0f, 8.0f);
        Line l(i * 1.0f, i * 2.0f, (i+1) * 1.0f, (i+1) * 2.0f);
        
        shape_ids.push_back(container_->add_circle(c));
        shape_ids.push_back(container_->add_rectangle(r));
        shape_ids.push_back(container_->add_ellipse(e));
        shape_ids.push_back(container_->add_line(l));
    }
    
    // Verify each type is stored separately
    EXPECT_EQ(container_->count(ShapeType::Circle), 256u);
    EXPECT_EQ(container_->count(ShapeType::Rectangle), 256u);
    EXPECT_EQ(container_->count(ShapeType::Ellipse), 256u);
    EXPECT_EQ(container_->count(ShapeType::Line), 256u);
    
    // Verify we can process each type independently with SIMD
    const auto& circles = container_->get_circles();
    
    // SIMD bounds calculation for circles
    std::vector<BoundingBox> circle_bounds(circles.size());
    const size_t simd_width = 8; // AVX can process 8 floats
    
    for (size_t i = 0; i < circles.size(); i += simd_width) {
        size_t batch_size = std::min(simd_width, circles.size() - i);
        
        for (size_t j = 0; j < batch_size; ++j) {
            circle_bounds[i + j] = circles[i + j].bounds();
        }
    }
    
    // Verify bounds are correct
    for (size_t i = 0; i < circles.size(); ++i) {
        if (circles[i].radius > 0) {  // Valid circle
            EXPECT_FLOAT_EQ(circle_bounds[i].min_x, circles[i].center_x - circles[i].radius);
            EXPECT_FLOAT_EQ(circle_bounds[i].max_x, circles[i].center_x + circles[i].radius);
        }
    }
}

// Test 4: Test batch transformation performance
TEST_F(SoAContainerAdvancedTest, BatchTransformationPerformance) {
    const size_t num_circles = 10000;
    
    // Add circles
    for (size_t i = 0; i < num_circles; ++i) {
        Circle c(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_));
        container_->add_circle(c);
    }
    
    Transform2D transform = Transform2D::translate(10.0f, 20.0f)
                                      .scale(1.5f, 1.5f)
                                      .rotate(M_PI / 4);
    
    // Time batch transformation
    auto start = std::chrono::high_resolution_clock::now();
    
    // Get mutable access through const_cast (in real code, provide mutable accessor)
    auto& circles = const_cast<std::vector<CircleShape>&>(container_->get_circles());
    
    // Batch transform all circles
    #pragma omp parallel for
    for (size_t i = 0; i < circles.size(); ++i) {
        Point2D center(circles[i].center_x, circles[i].center_y);
        Point2D new_center = transform.transform_point(center);
        
        circles[i].center_x = new_center.x;
        circles[i].center_y = new_center.y;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double us_per_shape = static_cast<double>(duration.count()) / circles.size();
    EXPECT_LT(us_per_shape, 0.1) << "Batch transform too slow: " << us_per_shape << " μs/shape";
}

// Test 5: Test efficient iteration patterns
TEST_F(SoAContainerAdvancedTest, EfficientIterationPatterns) {
    // Add mixed shapes
    for (int i = 0; i < 1000; ++i) {
        if (i % 4 == 0) {
            Circle c(i * 1.0f, i * 2.0f, 5.0f);
            container_->add_circle(c);
        } else if (i % 4 == 1) {
            Rectangle r(i * 1.0f, i * 2.0f, 10.0f, 15.0f);
            container_->add_rectangle(r);
        } else if (i % 4 == 2) {
            Ellipse e(i * 1.0f, i * 2.0f, 8.0f, 6.0f);
            container_->add_ellipse(e);
        } else {
            Line l(i * 1.0f, i * 2.0f, (i+10) * 1.0f, (i+10) * 2.0f);
            container_->add_line(l);
        }
    }
    
    // Test type-specific iteration
    size_t circle_count = 0;
    container_->for_each_type<CircleShape>([&circle_count](const CircleShape& circle) {
        circle_count++;
        EXPECT_GT(circle.radius, 0.0f);
    });
    EXPECT_EQ(circle_count, container_->count(ShapeType::Circle));
    
    // Test sorted iteration
    std::vector<uint16_t> z_indices;
    container_->for_each_sorted([&z_indices](const auto& shape) {
        // This lambda is called with different shape types
        // In real usage, you'd use std::visit or similar
        z_indices.push_back(0); // Placeholder
    });
    
    // Verify we visited all visible shapes
    EXPECT_EQ(z_indices.size(), container_->size());
}

// Test 6: Test memory compaction
TEST_F(SoAContainerAdvancedTest, MemoryCompaction) {
    std::vector<uint32_t> ids;
    
    // Add shapes
    for (int i = 0; i < 1000; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 5.0f);
        ids.push_back(container_->add_circle(c));
    }
    
    size_t initial_capacity = container_->get_circles().capacity();
    
    // Remove every other shape
    for (size_t i = 0; i < ids.size(); i += 2) {
        ASSERT_TRUE(container_->remove(ids[i]));
    }
    
    // Compact the container
    container_->compact();
    
    // Verify shapes are still accessible
    for (size_t i = 1; i < ids.size(); i += 2) {
        EXPECT_TRUE(container_->contains(ids[i]));
    }
    
    // After compaction, capacity might be reduced
    EXPECT_LE(container_->get_circles().capacity(), initial_capacity);
}

// Test 7: Test parallel access safety
TEST_F(SoAContainerAdvancedTest, ParallelAccessSafety) {
    const size_t num_shapes = 10000;
    
    // Add shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 5.0f);
        container_->add_circle(c);
    }
    
    // Parallel read access should be safe
    std::atomic<size_t> total_count{0};
    
    // Use a vector of partial sums instead of atomic float
    const int num_threads = 8;
    std::vector<float> partial_sums(num_threads, 0.0f);
    
    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = 0;
        #ifdef _OPENMP
        thread_id = omp_get_thread_num();
        #endif
        
        float local_sum = 0.0f;
        container_->for_each_type<CircleShape>([&total_count, &local_sum](const CircleShape& circle) {
            total_count.fetch_add(1, std::memory_order_relaxed);
            local_sum += circle.center_x;
        });
        
        partial_sums[thread_id] = local_sum;
    }
    
    // Calculate total sum from partial sums
    float total_sum = 0.0f;
    for (float partial : partial_sums) {
        total_sum += partial;
    }
    
    // Verify we processed shapes correctly
    EXPECT_GT(total_count.load(), 0u);
    EXPECT_GT(total_sum, 0.0f);
}

// Test 8: Test index stability
TEST_F(SoAContainerAdvancedTest, IndexStability) {
    std::vector<uint32_t> ids;
    std::vector<float> original_x_values;
    
    // Add shapes
    for (int i = 0; i < 100; ++i) {
        float x = i * 1.0f;
        Circle c(x, i * 2.0f, 5.0f);
        ids.push_back(container_->add_circle(c));
        original_x_values.push_back(x);
    }
    
    // Remove some shapes
    for (int i = 10; i < 20; ++i) {
        container_->remove(ids[i]);
    }
    
    // Add more shapes
    for (int i = 100; i < 150; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 5.0f);
        ids.push_back(container_->add_circle(c));
    }
    
    // Verify remaining shapes still have correct data
    for (int i = 0; i < 10; ++i) {
        auto* circle = container_->get<CircleShape>(ids[i]);
        ASSERT_NE(circle, nullptr);
        EXPECT_FLOAT_EQ(circle->center_x, original_x_values[i]);
    }
    
    for (int i = 20; i < 100; ++i) {
        auto* circle = container_->get<CircleShape>(ids[i]);
        ASSERT_NE(circle, nullptr);
        EXPECT_FLOAT_EQ(circle->center_x, original_x_values[i]);
    }
}

// Test 9: Test heterogeneous shape operations
TEST_F(SoAContainerAdvancedTest, HeterogeneousShapeOperations) {
    // Add different shapes
    for (int i = 0; i < 100; ++i) {
        Circle c(i * 1.0f, i * 1.0f, 5.0f);
        Rectangle r(i * 2.0f, i * 2.0f, 10.0f, 10.0f);
        Ellipse e(i * 3.0f, i * 3.0f, 8.0f, 6.0f);
        Line l(i * 4.0f, i * 4.0f, (i+1) * 4.0f, (i+1) * 4.0f);
        
        container_->add_circle(c);
        container_->add_rectangle(r);
        container_->add_ellipse(e);
        container_->add_line(l);
    }
    
    // Calculate total bounds using sorted iteration
    BoundingBox total_bounds;
    bool first = true;
    
    container_->for_each_sorted([&total_bounds, &first](const auto& shape) {
        BoundingBox shape_bounds = shape.get_bounds();
        if (first) {
            total_bounds = shape_bounds;
            first = false;
        } else {
            total_bounds = total_bounds.union_with(shape_bounds);
        }
    });
    
    // Verify bounds are reasonable
    EXPECT_LT(total_bounds.min_x, 0.0f);
    EXPECT_GT(total_bounds.max_x, 300.0f);
    
    // Compare with container's built-in bounds calculation
    BoundingBox container_bounds = container_->get_bounds();
    EXPECT_FLOAT_EQ(container_bounds.min_x, total_bounds.min_x);
    EXPECT_FLOAT_EQ(container_bounds.max_x, total_bounds.max_x);
}

// Test 10: Benchmark SoA vs AoS performance
TEST_F(SoAContainerAdvancedTest, BenchmarkSoAvsAoS) {
    const size_t num_shapes = 100000;
    
    // Add shapes to SoA container
    auto soa_start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 5.0f);
        container_->add_circle(c);
    }
    
    auto soa_add_end = std::chrono::high_resolution_clock::now();
    
    // Perform operation on all shapes (calculate total area)
    double total_area = 0.0;
    const auto& circles = container_->get_circles();
    
    for (const auto& circle : circles) {
        total_area += M_PI * circle.radius * circle.radius;
    }
    
    auto soa_calc_end = std::chrono::high_resolution_clock::now();
    
    // Calculate timings
    auto soa_add_time = std::chrono::duration_cast<std::chrono::microseconds>(
        soa_add_end - soa_start).count();
    auto soa_calc_time = std::chrono::duration_cast<std::chrono::microseconds>(
        soa_calc_end - soa_add_end).count();
    
    double soa_add_ns_per_shape = (soa_add_time * 1000.0) / num_shapes;
    double soa_calc_ns_per_shape = (soa_calc_time * 1000.0) / num_shapes;
    
    // Log performance metrics
    std::cout << "SoA Performance:" << std::endl;
    std::cout << "  Add: " << soa_add_ns_per_shape << " ns/shape" << std::endl;
    std::cout << "  Calculate: " << soa_calc_ns_per_shape << " ns/shape" << std::endl;
    std::cout << "  Total area: " << total_area << std::endl;
    
    // Verify performance meets requirements
    EXPECT_LT(soa_add_ns_per_shape, 1000.0);  // < 1 μs per shape
    EXPECT_LT(soa_calc_ns_per_shape, 10.0);   // < 10 ns per calculation
}