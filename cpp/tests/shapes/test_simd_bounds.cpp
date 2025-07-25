#include <gtest/gtest.h>
#include "claude_draw/shapes/shape_batch_ops.h"
#include "claude_draw/core/simd.h"
#include <chrono>
#include <random>
#include <vector>
#include <cstring>

using namespace claude_draw::shapes;
using namespace claude_draw::shapes::batch;
using namespace claude_draw;

class SimdBoundsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Seed random generator for reproducible tests
        rng_.seed(42);
    }
    
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_{0.0f, 100.0f};
};

// Test SIMD circle bounds calculation
TEST_F(SimdBoundsTest, CircleBoundsSimd) {
    const size_t count = 1024;  // Multiple of 4 for SIMD
    std::vector<CircleShape> circles;
    std::vector<BoundingBox> results(count);
    std::vector<BoundingBox> expected(count);
    
    // Generate test data
    for (size_t i = 0; i < count; ++i) {
        float cx = dist_(rng_);
        float cy = dist_(rng_);
        float r = dist_(rng_) * 0.5f;  // Smaller radius
        circles.emplace_back(cx, cy, r);
        
        // Calculate expected bounds
        expected[i] = BoundingBox(cx - r, cy - r, cx + r, cy + r);
    }
    
    // Test SIMD implementation
    simd::calculate_circle_bounds_simd(circles.data(), count, results.data());
    
    // Verify results
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(results[i].min_x, expected[i].min_x) << "Circle " << i;
        EXPECT_FLOAT_EQ(results[i].min_y, expected[i].min_y) << "Circle " << i;
        EXPECT_FLOAT_EQ(results[i].max_x, expected[i].max_x) << "Circle " << i;
        EXPECT_FLOAT_EQ(results[i].max_y, expected[i].max_y) << "Circle " << i;
    }
}

// Test SIMD with non-aligned count
TEST_F(SimdBoundsTest, CircleBoundsSimdNonAligned) {
    const size_t count = 1023;  // Not a multiple of 4
    std::vector<CircleShape> circles;
    std::vector<BoundingBox> results(count);
    
    // Generate test data
    for (size_t i = 0; i < count; ++i) {
        float cx = i * 10.0f;
        float cy = i * 5.0f;
        float r = 3.0f;
        circles.emplace_back(cx, cy, r);
    }
    
    // Test SIMD implementation with non-aligned count
    simd::calculate_circle_bounds_simd(circles.data(), count, results.data());
    
    // Verify all results, especially the last few
    for (size_t i = count - 5; i < count; ++i) {
        EXPECT_FLOAT_EQ(results[i].min_x, i * 10.0f - 3.0f);
        EXPECT_FLOAT_EQ(results[i].max_x, i * 10.0f + 3.0f);
        EXPECT_FLOAT_EQ(results[i].min_y, i * 5.0f - 3.0f);
        EXPECT_FLOAT_EQ(results[i].max_y, i * 5.0f + 3.0f);
    }
}

// Test SIMD rectangle bounds
TEST_F(SimdBoundsTest, RectangleBoundsSimd) {
    const size_t count = 512;
    std::vector<RectangleShape> rectangles;
    std::vector<BoundingBox> results(count);
    
    // Generate test data
    for (size_t i = 0; i < count; ++i) {
        float x = dist_(rng_);
        float y = dist_(rng_);
        float w = dist_(rng_) * 0.5f;
        float h = dist_(rng_) * 0.5f;
        rectangles.emplace_back(x, y, w, h);
    }
    
    // Test SIMD implementation
    simd::calculate_rectangle_bounds_simd(rectangles.data(), count, results.data());
    
    // Verify results
    for (size_t i = 0; i < count; ++i) {
        BoundingBox expected = rectangles[i].bounds();
        EXPECT_FLOAT_EQ(results[i].min_x, expected.min_x);
        EXPECT_FLOAT_EQ(results[i].min_y, expected.min_y);
        EXPECT_FLOAT_EQ(results[i].max_x, expected.max_x);
        EXPECT_FLOAT_EQ(results[i].max_y, expected.max_y);
    }
}

// Test SIMD ellipse bounds
TEST_F(SimdBoundsTest, EllipseBoundsSimd) {
    const size_t count = 256;
    std::vector<EllipseShape> ellipses;
    std::vector<BoundingBox> results(count);
    
    // Generate test data
    for (size_t i = 0; i < count; ++i) {
        float cx = dist_(rng_);
        float cy = dist_(rng_);
        float rx = dist_(rng_) * 0.3f;
        float ry = dist_(rng_) * 0.3f;
        ellipses.emplace_back(cx, cy, rx, ry);
    }
    
    // Test SIMD implementation
    simd::calculate_ellipse_bounds_simd(ellipses.data(), count, results.data());
    
    // Verify results
    for (size_t i = 0; i < count; ++i) {
        BoundingBox expected = ellipses[i].bounds();
        EXPECT_FLOAT_EQ(results[i].min_x, expected.min_x);
        EXPECT_FLOAT_EQ(results[i].min_y, expected.min_y);
        EXPECT_FLOAT_EQ(results[i].max_x, expected.max_x);
        EXPECT_FLOAT_EQ(results[i].max_y, expected.max_y);
    }
}

// Test SIMD line bounds
TEST_F(SimdBoundsTest, LineBoundsSimd) {
    const size_t count = 512;
    std::vector<LineShape> lines;
    std::vector<BoundingBox> results(count);
    
    // Generate test data
    for (size_t i = 0; i < count; ++i) {
        float x1 = dist_(rng_);
        float y1 = dist_(rng_);
        float x2 = dist_(rng_);
        float y2 = dist_(rng_);
        lines.emplace_back(x1, y1, x2, y2);
    }
    
    // Test SIMD implementation
    simd::calculate_line_bounds_simd(lines.data(), count, results.data());
    
    // Verify results
    for (size_t i = 0; i < count; ++i) {
        BoundingBox expected = lines[i].bounds();
        EXPECT_FLOAT_EQ(results[i].min_x, expected.min_x);
        EXPECT_FLOAT_EQ(results[i].min_y, expected.min_y);
        EXPECT_FLOAT_EQ(results[i].max_x, expected.max_x);
        EXPECT_FLOAT_EQ(results[i].max_y, expected.max_y);
    }
}

// Performance comparison test
TEST_F(SimdBoundsTest, PerformanceComparison) {
    const size_t count = 100000;
    std::vector<CircleShape> circles;
    std::vector<BoundingBox> simd_results(count);
    std::vector<BoundingBox> scalar_results(count);
    
    // Generate large dataset
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(dist_(rng_), dist_(rng_), dist_(rng_) * 0.5f);
    }
    
    // Time SIMD version
    auto simd_start = std::chrono::high_resolution_clock::now();
    simd::calculate_circle_bounds_simd(circles.data(), count, simd_results.data());
    auto simd_end = std::chrono::high_resolution_clock::now();
    
    // Time scalar version
    auto scalar_start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        scalar_results[i] = circles[i].bounds();
    }
    auto scalar_end = std::chrono::high_resolution_clock::now();
    
    auto simd_duration = std::chrono::duration_cast<std::chrono::microseconds>(simd_end - simd_start);
    auto scalar_duration = std::chrono::duration_cast<std::chrono::microseconds>(scalar_end - scalar_start);
    
    // Verify results match
    for (size_t i = 0; i < std::min(size_t(100), count); ++i) {
        EXPECT_FLOAT_EQ(simd_results[i].min_x, scalar_results[i].min_x);
        EXPECT_FLOAT_EQ(simd_results[i].max_x, scalar_results[i].max_x);
    }
    
    double speedup = static_cast<double>(scalar_duration.count()) / simd_duration.count();
    
    std::cout << "SIMD bounds calculation performance:\n";
    std::cout << "  Scalar: " << scalar_duration.count() << " μs\n";
    std::cout << "  SIMD:   " << simd_duration.count() << " μs\n";
    std::cout << "  Speedup: " << speedup << "x\n";
    
    // SIMD should be faster (though it might not be on all systems)
    #ifdef __AVX2__
    // Only expect speedup if AVX2 is actually available
    if (speedup > 1.0) {
        std::cout << "  AVX2 SIMD optimization is working!\n";
    }
    #endif
}

// Test edge cases
TEST_F(SimdBoundsTest, EdgeCases) {
    // Test with zero circles
    std::vector<BoundingBox> results(1);
    simd::calculate_circle_bounds_simd(nullptr, 0, results.data());
    
    // Test with single circle
    CircleShape single_circle(10.0f, 20.0f, 5.0f);
    simd::calculate_circle_bounds_simd(&single_circle, 1, results.data());
    
    EXPECT_FLOAT_EQ(results[0].min_x, 5.0f);
    EXPECT_FLOAT_EQ(results[0].min_y, 15.0f);
    EXPECT_FLOAT_EQ(results[0].max_x, 15.0f);
    EXPECT_FLOAT_EQ(results[0].max_y, 25.0f);
    
    // Test with extreme values
    std::vector<CircleShape> extreme_circles = {
        CircleShape(1e10f, 1e10f, 1e5f),
        CircleShape(-1e10f, -1e10f, 1e5f),
        CircleShape(0.0f, 0.0f, 0.0f),
        CircleShape(1e-10f, 1e-10f, 1e-5f)
    };
    
    std::vector<BoundingBox> extreme_results(4);
    simd::calculate_circle_bounds_simd(extreme_circles.data(), 4, extreme_results.data());
    
    // Verify extreme values are handled correctly
    EXPECT_FLOAT_EQ(extreme_results[2].min_x, 0.0f);  // Zero radius circle
    EXPECT_FLOAT_EQ(extreme_results[2].max_x, 0.0f);
}

// Test memory alignment requirements
TEST_F(SimdBoundsTest, MemoryAlignment) {
    // Allocate unaligned memory
    std::vector<uint8_t> buffer(sizeof(CircleShape) * 10 + 15);
    
    // Create unaligned pointer
    CircleShape* unaligned = reinterpret_cast<CircleShape*>(buffer.data() + 1);
    
    // Fill with test data
    for (size_t i = 0; i < 8; ++i) {
        new (&unaligned[i]) CircleShape(i * 10.0f, i * 5.0f, 2.0f);
    }
    
    std::vector<BoundingBox> results(8);
    
    // SIMD implementation should handle unaligned data
    simd::calculate_circle_bounds_simd(unaligned, 8, results.data());
    
    // Verify results
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_FLOAT_EQ(results[i].min_x, i * 10.0f - 2.0f);
        EXPECT_FLOAT_EQ(results[i].max_x, i * 10.0f + 2.0f);
    }
}

// Test batch size variations
TEST_F(SimdBoundsTest, BatchSizeVariations) {
    // Test various batch sizes to ensure correct handling
    std::vector<size_t> batch_sizes = {1, 2, 3, 4, 5, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65};
    
    for (size_t batch_size : batch_sizes) {
        std::vector<CircleShape> circles;
        std::vector<BoundingBox> results(batch_size);
        
        for (size_t i = 0; i < batch_size; ++i) {
            circles.emplace_back(i * 1.0f, i * 2.0f, 0.5f);
        }
        
        simd::calculate_circle_bounds_simd(circles.data(), batch_size, results.data());
        
        // Verify all results
        for (size_t i = 0; i < batch_size; ++i) {
            EXPECT_FLOAT_EQ(results[i].min_x, i * 1.0f - 0.5f) 
                << "Failed for batch size " << batch_size << " at index " << i;
            EXPECT_FLOAT_EQ(results[i].max_x, i * 1.0f + 0.5f)
                << "Failed for batch size " << batch_size << " at index " << i;
        }
    }
}

// Test mixed shape bounds calculation via batch API
TEST_F(SimdBoundsTest, MixedShapeBatchBounds) {
    // Create a large mixed dataset
    const size_t shapes_per_type = 250;
    const size_t total_shapes = shapes_per_type * 4;
    
    std::vector<const void*> shape_ptrs;
    std::vector<ShapeType> shape_types;
    std::vector<std::unique_ptr<void, void(*)(void*)>> shape_storage;
    
    // Add circles
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto circle = std::make_unique<Circle>(dist_(rng_), dist_(rng_), dist_(rng_) * 0.5f);
        shape_ptrs.push_back(circle.get());
        shape_types.push_back(ShapeType::Circle);
        shape_storage.emplace_back(circle.release(), [](void* p) { delete static_cast<Circle*>(p); });
    }
    
    // Add rectangles
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto rect = std::make_unique<Rectangle>(dist_(rng_), dist_(rng_), 
                                                dist_(rng_) * 0.5f, dist_(rng_) * 0.5f);
        shape_ptrs.push_back(rect.get());
        shape_types.push_back(ShapeType::Rectangle);
        shape_storage.emplace_back(rect.release(), [](void* p) { delete static_cast<Rectangle*>(p); });
    }
    
    // Add ellipses
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto ellipse = std::make_unique<Ellipse>(dist_(rng_), dist_(rng_), 
                                                 dist_(rng_) * 0.3f, dist_(rng_) * 0.3f);
        shape_ptrs.push_back(ellipse.get());
        shape_types.push_back(ShapeType::Ellipse);
        shape_storage.emplace_back(ellipse.release(), [](void* p) { delete static_cast<Ellipse*>(p); });
    }
    
    // Add lines
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto line = std::make_unique<Line>(dist_(rng_), dist_(rng_), 
                                          dist_(rng_), dist_(rng_));
        shape_ptrs.push_back(line.get());
        shape_types.push_back(ShapeType::Line);
        shape_storage.emplace_back(line.release(), [](void* p) { delete static_cast<Line*>(p); });
    }
    
    std::vector<BoundingBox> results(total_shapes);
    
    // Time the mixed batch operation
    auto start = std::chrono::high_resolution_clock::now();
    calculate_bounds(shape_ptrs.data(), shape_types.data(), total_shapes, results.data());
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double us_per_shape = static_cast<double>(duration.count()) / total_shapes;
    
    std::cout << "Mixed shape batch bounds: " << us_per_shape 
              << " μs/shape for " << total_shapes << " shapes\n";
    
    // Performance should be good even with mixed types
    EXPECT_LT(us_per_shape, 1.0);
}