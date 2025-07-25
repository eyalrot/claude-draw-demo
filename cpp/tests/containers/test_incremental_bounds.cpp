#include <gtest/gtest.h>
#include "claude_draw/containers/incremental_bounds.h"
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using namespace claude_draw::containers;
using namespace claude_draw;

class IncrementalBoundsTest : public ::testing::Test {
protected:
    void SetUp() override {
        bounds_ = std::make_unique<IncrementalBounds>();
    }
    
    std::unique_ptr<IncrementalBounds> bounds_;
};

// Test empty bounds
TEST_F(IncrementalBoundsTest, EmptyBounds) {
    EXPECT_EQ(bounds_->shape_count(), 0u);
    BoundingBox empty = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(empty.min_x, 0.0f);
    EXPECT_FLOAT_EQ(empty.min_y, 0.0f);
    EXPECT_FLOAT_EQ(empty.max_x, 0.0f);
    EXPECT_FLOAT_EQ(empty.max_y, 0.0f);
}

// Test single shape addition
TEST_F(IncrementalBoundsTest, SingleShapeAddition) {
    BoundingBox shape(10.0f, 20.0f, 30.0f, 40.0f);
    bounds_->add_shape(shape);
    
    EXPECT_EQ(bounds_->shape_count(), 1u);
    EXPECT_FALSE(bounds_->is_dirty());
    
    BoundingBox result = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(result.min_x, 10.0f);
    EXPECT_FLOAT_EQ(result.min_y, 20.0f);
    EXPECT_FLOAT_EQ(result.max_x, 30.0f);
    EXPECT_FLOAT_EQ(result.max_y, 40.0f);
}

// Test multiple shape additions
TEST_F(IncrementalBoundsTest, MultipleShapeAdditions) {
    bounds_->add_shape(BoundingBox(0.0f, 0.0f, 10.0f, 10.0f));
    bounds_->add_shape(BoundingBox(5.0f, 5.0f, 15.0f, 15.0f));
    bounds_->add_shape(BoundingBox(-5.0f, -5.0f, 5.0f, 5.0f));
    
    EXPECT_EQ(bounds_->shape_count(), 3u);
    
    BoundingBox result = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(result.min_x, -5.0f);
    EXPECT_FLOAT_EQ(result.min_y, -5.0f);
    EXPECT_FLOAT_EQ(result.max_x, 15.0f);
    EXPECT_FLOAT_EQ(result.max_y, 15.0f);
}

// Test shape removal - dirty flag
TEST_F(IncrementalBoundsTest, ShapeRemovalDirtyFlag) {
    // Add shapes
    BoundingBox edge_shape(0.0f, 0.0f, 100.0f, 100.0f);
    BoundingBox inner_shape(40.0f, 40.0f, 60.0f, 60.0f);
    
    bounds_->add_shape(edge_shape);
    bounds_->add_shape(inner_shape);
    
    // Remove inner shape - should not mark dirty
    bounds_->remove_shape(inner_shape);
    EXPECT_FALSE(bounds_->is_dirty());
    EXPECT_EQ(bounds_->shape_count(), 1u);
    
    // Remove edge shape - should mark dirty
    bounds_->remove_shape(edge_shape);
    EXPECT_FALSE(bounds_->is_dirty());  // Empty bounds are not dirty
    EXPECT_EQ(bounds_->shape_count(), 0u);
    
    // Verify bounds are reset to empty
    BoundingBox result = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(result.min_x, 0.0f);
    EXPECT_FLOAT_EQ(result.min_y, 0.0f);
    EXPECT_FLOAT_EQ(result.max_x, 0.0f);
    EXPECT_FLOAT_EQ(result.max_y, 0.0f);
}

// Test shape update
TEST_F(IncrementalBoundsTest, ShapeUpdate) {
    BoundingBox original(10.0f, 10.0f, 20.0f, 20.0f);
    bounds_->add_shape(original);
    
    // Update to smaller bounds within original
    BoundingBox smaller(12.0f, 12.0f, 18.0f, 18.0f);
    bounds_->update_shape(original, smaller);
    EXPECT_FALSE(bounds_->is_dirty());
    
    // Update to bounds extending beyond original
    BoundingBox larger(5.0f, 5.0f, 25.0f, 25.0f);
    bounds_->update_shape(smaller, larger);
    
    BoundingBox result = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(result.min_x, 5.0f);
    EXPECT_FLOAT_EQ(result.min_y, 5.0f);
    EXPECT_FLOAT_EQ(result.max_x, 25.0f);
    EXPECT_FLOAT_EQ(result.max_y, 25.0f);
}

// Test forced recalculation
TEST_F(IncrementalBoundsTest, ForcedRecalculation) {
    bounds_->add_shape(BoundingBox(0.0f, 0.0f, 50.0f, 50.0f));
    bounds_->add_shape(BoundingBox(25.0f, 25.0f, 75.0f, 75.0f));
    
    // Mark dirty and set new bounds
    bounds_->mark_dirty();
    EXPECT_TRUE(bounds_->is_dirty());
    
    BoundingBox new_bounds(10.0f, 10.0f, 60.0f, 60.0f);
    bounds_->set_bounds(new_bounds);
    EXPECT_FALSE(bounds_->is_dirty());
    
    BoundingBox result = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(result.min_x, 10.0f);
    EXPECT_FLOAT_EQ(result.min_y, 10.0f);
    EXPECT_FLOAT_EQ(result.max_x, 60.0f);
    EXPECT_FLOAT_EQ(result.max_y, 60.0f);
}

// Test clear operation
TEST_F(IncrementalBoundsTest, ClearOperation) {
    bounds_->add_shape(BoundingBox(0.0f, 0.0f, 100.0f, 100.0f));
    bounds_->add_shape(BoundingBox(-50.0f, -50.0f, 50.0f, 50.0f));
    
    EXPECT_EQ(bounds_->shape_count(), 2u);
    
    bounds_->clear();
    
    EXPECT_EQ(bounds_->shape_count(), 0u);
    EXPECT_FALSE(bounds_->is_dirty());
    
    BoundingBox result = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(result.min_x, 0.0f);
    EXPECT_FLOAT_EQ(result.min_y, 0.0f);
    EXPECT_FLOAT_EQ(result.max_x, 0.0f);
    EXPECT_FLOAT_EQ(result.max_y, 0.0f);
}

// Test thread safety
TEST_F(IncrementalBoundsTest, ThreadSafety) {
    const int num_threads = 4;
    const int shapes_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, shapes_per_thread]() {
            std::mt19937 rng(t);
            std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
            
            for (int i = 0; i < shapes_per_thread; ++i) {
                float x1 = dist(rng);
                float y1 = dist(rng);
                float x2 = x1 + std::abs(dist(rng));
                float y2 = y1 + std::abs(dist(rng));
                
                bounds_->add_shape(BoundingBox(x1, y1, x2, y2));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(bounds_->shape_count(), 
              static_cast<size_t>(num_threads * shapes_per_thread));
}

// Test incremental performance
TEST_F(IncrementalBoundsTest, IncrementalPerformance) {
    const size_t num_shapes = 10000;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    
    // Measure incremental addition time
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < num_shapes; ++i) {
        float x = dist(rng);
        float y = dist(rng);
        float w = std::abs(dist(rng)) * 0.1f;
        float h = std::abs(dist(rng)) * 0.1f;
        bounds_->add_shape(BoundingBox(x, y, x + w, y + h));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto add_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Incremental addition: " << add_time.count() / 1000.0 
              << " ms for " << num_shapes << " shapes ("
              << add_time.count() / static_cast<double>(num_shapes) 
              << " μs/shape)\n";
    
    // Measure bounds retrieval time
    start = std::chrono::high_resolution_clock::now();
    BoundingBox result = bounds_->get_bounds();
    end = std::chrono::high_resolution_clock::now();
    auto get_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "Bounds retrieval: " << get_time.count() << " ns\n";
    
    // Verify bounds are reasonable
    EXPECT_LT(result.min_x, -900.0f);
    EXPECT_LT(result.min_y, -900.0f);
    EXPECT_GT(result.max_x, 900.0f);
    EXPECT_GT(result.max_y, 900.0f);
}

// Test edge shape removal
TEST_F(IncrementalBoundsTest, EdgeShapeRemoval) {
    // Create a scenario where removing one shape affects bounds
    bounds_->add_shape(BoundingBox(0.0f, 0.0f, 100.0f, 100.0f));
    bounds_->add_shape(BoundingBox(50.0f, 50.0f, 150.0f, 150.0f));  // Extends bounds
    bounds_->add_shape(BoundingBox(25.0f, 25.0f, 75.0f, 75.0f));    // Inner shape
    
    BoundingBox initial = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(initial.max_x, 150.0f);
    EXPECT_FLOAT_EQ(initial.max_y, 150.0f);
    
    // Remove the shape that extends bounds
    bounds_->remove_shape(BoundingBox(50.0f, 50.0f, 150.0f, 150.0f));
    EXPECT_TRUE(bounds_->is_dirty());
    
    // Simulate recalculation (in real usage, container would do this)
    bounds_->set_bounds(BoundingBox(0.0f, 0.0f, 100.0f, 100.0f));
    
    BoundingBox after_removal = bounds_->get_bounds();
    EXPECT_FLOAT_EQ(after_removal.max_x, 100.0f);
    EXPECT_FLOAT_EQ(after_removal.max_y, 100.0f);
}

// HierarchicalBounds tests
class HierarchicalBoundsTest : public ::testing::Test {
protected:
    void SetUp() override {
        h_bounds_ = std::make_unique<HierarchicalBounds>();
    }
    
    std::unique_ptr<HierarchicalBounds> h_bounds_;
};

// Test basic hierarchical bounds
TEST_F(HierarchicalBoundsTest, BasicHierarchicalBounds) {
    // Add shapes to different regions
    h_bounds_->add_shape(BoundingBox(0.0f, 0.0f, 10.0f, 10.0f));      // Top-left
    h_bounds_->add_shape(BoundingBox(7000.0f, 7000.0f, 7100.0f, 7100.0f)); // Bottom-right
    h_bounds_->add_shape(BoundingBox(3500.0f, 3500.0f, 3600.0f, 3600.0f)); // Center
    
    // Get global bounds
    BoundingBox global = h_bounds_->get_bounds();
    EXPECT_FLOAT_EQ(global.min_x, 0.0f);
    EXPECT_FLOAT_EQ(global.min_y, 0.0f);
    EXPECT_FLOAT_EQ(global.max_x, 7100.0f);
    EXPECT_FLOAT_EQ(global.max_y, 7100.0f);
}

// Test region queries
TEST_F(HierarchicalBoundsTest, RegionQueries) {
    // Add shapes in specific regions
    h_bounds_->add_shape(BoundingBox(100.0f, 100.0f, 200.0f, 200.0f));
    h_bounds_->add_shape(BoundingBox(150.0f, 150.0f, 250.0f, 250.0f));
    h_bounds_->add_shape(BoundingBox(5000.0f, 5000.0f, 5100.0f, 5100.0f));
    
    // Query small region
    BoundingBox small_region(0.0f, 0.0f, 1000.0f, 1000.0f);
    BoundingBox small_bounds = h_bounds_->get_region_bounds(small_region);
    EXPECT_FLOAT_EQ(small_bounds.min_x, 100.0f);
    EXPECT_FLOAT_EQ(small_bounds.min_y, 100.0f);
    EXPECT_FLOAT_EQ(small_bounds.max_x, 250.0f);
    EXPECT_FLOAT_EQ(small_bounds.max_y, 250.0f);
    
    // Query large region
    BoundingBox large_region(0.0f, 0.0f, 8000.0f, 8000.0f);
    BoundingBox large_bounds = h_bounds_->get_region_bounds(large_region);
    EXPECT_FLOAT_EQ(large_bounds.min_x, 100.0f);
    EXPECT_FLOAT_EQ(large_bounds.min_y, 100.0f);
    EXPECT_FLOAT_EQ(large_bounds.max_x, 5100.0f);
    EXPECT_FLOAT_EQ(large_bounds.max_y, 5100.0f);
}

// Test hierarchical performance
TEST_F(HierarchicalBoundsTest, HierarchicalPerformance) {
    const size_t num_shapes = 10000;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 8000.0f);
    
    // Add shapes distributed across grid
    for (size_t i = 0; i < num_shapes; ++i) {
        float x = dist(rng);
        float y = dist(rng);
        float w = 10.0f + (i % 50);
        float h = 10.0f + (i % 50);
        h_bounds_->add_shape(BoundingBox(x, y, x + w, y + h));
    }
    
    // Measure region query performance
    std::vector<BoundingBox> test_regions = {
        BoundingBox(0.0f, 0.0f, 1000.0f, 1000.0f),      // Small region
        BoundingBox(2000.0f, 2000.0f, 4000.0f, 4000.0f), // Medium region
        BoundingBox(0.0f, 0.0f, 8000.0f, 8000.0f)        // Full region
    };
    
    for (const auto& region : test_regions) {
        auto start = std::chrono::high_resolution_clock::now();
        BoundingBox bounds = h_bounds_->get_region_bounds(region);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        std::cout << "Region query [" << region.width() << "x" << region.height() 
                  << "]: " << duration.count() << " ns\n";
    }
}

// Test clear operation
TEST_F(HierarchicalBoundsTest, ClearOperation) {
    h_bounds_->add_shape(BoundingBox(100.0f, 100.0f, 200.0f, 200.0f));
    h_bounds_->add_shape(BoundingBox(5000.0f, 5000.0f, 5100.0f, 5100.0f));
    
    h_bounds_->clear();
    
    BoundingBox bounds = h_bounds_->get_bounds();
    // Empty bounds should be (0, 0, 0, 0)
    EXPECT_FLOAT_EQ(bounds.min_x, 0.0f);
    EXPECT_FLOAT_EQ(bounds.min_y, 0.0f);
    EXPECT_FLOAT_EQ(bounds.max_x, 0.0f);
    EXPECT_FLOAT_EQ(bounds.max_y, 0.0f);
}