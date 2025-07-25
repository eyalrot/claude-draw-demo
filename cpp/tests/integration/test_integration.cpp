#include <gtest/gtest.h>
#include <vector>
#include <random>
#include <chrono>

#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"
#include "claude_draw/memory/type_pools.h"
#include "claude_draw/batch/batch_operations.h"

using namespace claude_draw;
using namespace claude_draw::memory;
using namespace claude_draw::batch;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear pools before each test
        TypePools::instance().clear_all();
    }
};

TEST_F(IntegrationTest, TransformChain) {
    // Test complex transformation chain
    Point2D p(10.0f, 20.0f);
    
    // Create transformation chain
    Transform2D t1 = Transform2D::translate(5.0f, 10.0f);
    Transform2D t2 = Transform2D::rotate(M_PI / 4);  // 45 degrees
    Transform2D t3 = Transform2D::scale(2.0f, 2.0f);
    Transform2D t4 = Transform2D::translate(-5.0f, -10.0f);
    
    // Compose transforms
    Transform2D combined = t4 * t3 * t2 * t1;
    
    // Apply to point
    Point2D result = combined.transform_point(p);
    
    // Apply step by step for verification
    Point2D step1 = t1.transform_point(p);
    Point2D step2 = t2.transform_point(step1);
    Point2D step3 = t3.transform_point(step2);
    Point2D step4 = t4.transform_point(step3);
    
    // Should get same result
    EXPECT_NEAR(result.x, step4.x, 1e-5f);
    EXPECT_NEAR(result.y, step4.y, 1e-5f);
}

TEST_F(IntegrationTest, ColorBlendingChain) {
    // Test multiple color blending operations
    Color background(255, 255, 255, 255);  // White
    Color layer1(255, 0, 0, 128);          // Semi-transparent red
    Color layer2(0, 255, 0, 128);          // Semi-transparent green
    Color layer3(0, 0, 255, 64);           // Quarter-transparent blue
    
    // Blend layers
    Color result1 = layer1.blend_over(background);
    Color result2 = layer2.blend_over(result1);
    Color result3 = layer3.blend_over(result2);
    
    // Final color should have components from all layers
    EXPECT_GT(result3.r, 0);
    EXPECT_GT(result3.g, 0);
    EXPECT_GT(result3.b, 0);
    EXPECT_EQ(result3.a, 255);  // Should be opaque
}

TEST_F(IntegrationTest, BoundingBoxTransformations) {
    // Test bounding box with transformations
    BoundingBox box(0.0f, 0.0f, 10.0f, 10.0f);
    
    // Get corners
    Point2D corners[4] = {
        Point2D(box.min_x, box.min_y),
        Point2D(box.max_x, box.min_y),
        Point2D(box.max_x, box.max_y),
        Point2D(box.min_x, box.max_y)
    };
    
    // Transform corners
    Transform2D transform = Transform2D::rotate(M_PI / 6) * Transform2D::scale(2.0f, 1.5f);
    
    BoundingBox transformed_box;
    for (int i = 0; i < 4; ++i) {
        Point2D transformed = transform.transform_point(corners[i]);
        transformed_box.expand(transformed);
    }
    
    // Verify transformed box contains all transformed corners
    for (int i = 0; i < 4; ++i) {
        Point2D transformed = transform.transform_point(corners[i]);
        EXPECT_TRUE(transformed_box.contains(transformed));
    }
}

TEST_F(IntegrationTest, MemoryPoolIntegration) {
    // Test using memory pools with batch operations
    const size_t count = 10000;
    
    // Pre-allocate pools
    TypePools& pools = TypePools::instance();
    pools.reserve_points(count);
    pools.reserve_colors(count);
    
    // Create objects using pools
    std::vector<PooledPtr<Point2D, TypePools::POINT_BLOCK_SIZE>> points;
    std::vector<PooledPtr<Color, TypePools::COLOR_BLOCK_SIZE>> colors;
    
    points.reserve(count);
    colors.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        points.push_back(TypePools::make_point(
            static_cast<float>(i % 100), 
            static_cast<float>(i % 200)
        ));
        colors.push_back(TypePools::make_color(
            static_cast<uint8_t>(i % 256),
            static_cast<uint8_t>((i * 2) % 256),
            static_cast<uint8_t>((i * 3) % 256),
            255
        ));
    }
    
    // Verify pool usage
    auto point_stats = pools.point_stats();
    auto color_stats = pools.color_stats();
    
    EXPECT_GE(point_stats.allocated, count);
    EXPECT_GE(color_stats.allocated, count);
}

TEST_F(IntegrationTest, BatchTransformIntegration) {
    // Test batch operations with transforms and bounding boxes
    const size_t count = 1000;
    
    // Create batch of points
    ContiguousBatch<Point2D> points(count);
    std::uniform_real_distribution<float> dist(-50.0f, 50.0f);
    std::mt19937 rng(42);
    
    for (size_t i = 0; i < count; ++i) {
        points.emplace(dist(rng), dist(rng));
    }
    
    // Create bounding box from points
    BoundingBox original_bbox;
    for (const auto& p : points.as_span()) {
        original_bbox.expand(p);
    }
    
    // Transform all points
    Transform2D transform = Transform2D::rotate(M_PI / 3) * 
                           Transform2D::scale(1.5f, 0.8f) *
                           Transform2D::translate(10.0f, -20.0f);
    
    BatchProcessor::transform_points(transform, points.as_span());
    
    // Create new bounding box
    BoundingBox transformed_bbox;
    for (const auto& p : points.as_span()) {
        transformed_bbox.expand(p);
    }
    
    // Test containment
    std::vector<bool> contains(count);
    BatchProcessor::contains_points(transformed_bbox, points.data(), contains.data(), count);
    
    // All points should be contained
    for (size_t i = 0; i < count; ++i) {
        EXPECT_TRUE(contains[i]) << "Point " << i << " not contained";
    }
}

TEST_F(IntegrationTest, RealWorldScenario) {
    // Simulate a real-world graphics operation
    // Drawing multiple colored shapes with transformations
    
    struct Shape {
        std::vector<Point2D> vertices;
        Color color;
        Transform2D transform;
        BoundingBox bounds;
    };
    
    // Create shapes
    std::vector<Shape> shapes;
    shapes.reserve(100);
    
    // Create 100 random rectangles
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> pos_dist(-100.0f, 100.0f);
    std::uniform_real_distribution<float> size_dist(5.0f, 20.0f);
    std::uniform_real_distribution<float> angle_dist(0.0f, 2 * M_PI);
    std::uniform_int_distribution<uint8_t> color_dist(0, 255);
    
    for (int i = 0; i < 100; ++i) {
        Shape shape;
        
        // Rectangle vertices
        float w = size_dist(rng);
        float h = size_dist(rng);
        shape.vertices = {
            Point2D(0, 0),
            Point2D(w, 0),
            Point2D(w, h),
            Point2D(0, h)
        };
        
        // Random color
        shape.color = Color(
            color_dist(rng),
            color_dist(rng),
            color_dist(rng),
            200  // Slightly transparent
        );
        
        // Random transform
        shape.transform = Transform2D::translate(pos_dist(rng), pos_dist(rng)) *
                         Transform2D::rotate(angle_dist(rng)) *
                         Transform2D::scale(
                             std::uniform_real_distribution<float>(0.5f, 2.0f)(rng),
                             std::uniform_real_distribution<float>(0.5f, 2.0f)(rng)
                         );
        
        // Calculate bounds
        shape.bounds = BoundingBox();
        for (const auto& v : shape.vertices) {
            Point2D transformed = shape.transform.transform_point(v);
            shape.bounds.expand(transformed);
        }
        
        shapes.push_back(shape);
    }
    
    // Find overlapping shapes
    int overlaps = 0;
    for (size_t i = 0; i < shapes.size(); ++i) {
        for (size_t j = i + 1; j < shapes.size(); ++j) {
            if (shapes[i].bounds.intersects(shapes[j].bounds)) {
                overlaps++;
            }
        }
    }
    
    std::cout << "Found " << overlaps << " overlapping shape pairs" << std::endl;
    
    // Composite all shapes
    Color canvas(255, 255, 255, 255);  // White background
    
    // Simple compositing (would be per-pixel in real renderer)
    for (const auto& shape : shapes) {
        canvas = shape.color.blend_over(canvas);
    }
    
    // Canvas should have mixed colors
    EXPECT_LT(canvas.r, 255);
    EXPECT_LT(canvas.g, 255);
    EXPECT_LT(canvas.b, 255);
}

TEST_F(IntegrationTest, PerformanceStressTest) {
    // Stress test with large number of operations
    const size_t object_count = 100000;
    const size_t iteration_count = 10;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t iter = 0; iter < iteration_count; ++iter) {
        // Create objects
        auto points = BatchCreator::create_points(object_count, 0.0f, 0.0f);
        auto colors = BatchCreator::create_colors(object_count, 0xFF000000);
        
        // Transform points
        Transform2D transform = Transform2D::rotate(0.1f) * Transform2D::scale(1.1f, 0.9f);
        
        for (auto* p : points) {
            *p = transform.transform_point(*p);
        }
        
        // Blend colors
        Color background(255, 255, 255, 255);
        for (auto* c : colors) {
            *c = c->blend_over(background);
        }
        
        // Calculate bounding box
        BoundingBox bbox;
        for (auto* p : points) {
            bbox.expand(*p);
        }
        
        // Clean up
        BatchCreator::destroy_batch(points);
        BatchCreator::destroy_batch(colors);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Stress test completed in " << duration.count() << " ms" << std::endl;
    std::cout << "Average per iteration: " << duration.count() / iteration_count << " ms" << std::endl;
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 10000);  // Less than 10 seconds
}

TEST_F(IntegrationTest, EdgeCasesAndErrorHandling) {
    // Test edge cases with all components
    
    // Empty bounding box
    BoundingBox empty_box;
    EXPECT_TRUE(empty_box.is_empty());
    
    // Transform empty box corners
    Transform2D t = Transform2D::scale(2.0f, 2.0f);
    Point2D p(empty_box.min_x, empty_box.min_y);
    Point2D transformed = t.transform_point(p);
    // Should handle gracefully
    
    // Degenerate transform
    Transform2D degenerate = Transform2D::scale(0.0f, 1.0f);
    EXPECT_FALSE(degenerate.is_invertible());
    
    // Zero-alpha color blending
    Color transparent(255, 0, 0, 0);
    Color opaque(0, 0, 255, 255);
    Color result = transparent.blend_over(opaque);
    EXPECT_EQ(result.rgba, opaque.rgba);  // Transparent has no effect
    
    // Very small distances
    Point2D p1(0.0f, 0.0f);
    Point2D p2(1e-7f, 1e-7f);
    float dist = p1.distance_to(p2);
    EXPECT_GT(dist, 0.0f);
    EXPECT_LT(dist, 1e-6f);
    
    // Large batch operations
    const size_t huge_count = 1000000;
    try {
        auto huge_batch = BatchCreator::create_points(huge_count, 0.0f, 0.0f);
        EXPECT_EQ(huge_batch.size(), huge_count);
        BatchCreator::destroy_batch(huge_batch);
    } catch (const std::bad_alloc&) {
        // Acceptable if system runs out of memory
        std::cout << "Large allocation failed (expected on low-memory systems)" << std::endl;
    }
}