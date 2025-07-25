#include <gtest/gtest.h>
#include "claude_draw/containers/soa_container.h"
#include <chrono>
#include <random>
#include <thread>
#include <type_traits>

using namespace claude_draw::containers;
using namespace claude_draw::shapes;
using namespace claude_draw;

class SoAContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<SoAContainer>();
    }
    
    std::unique_ptr<SoAContainer> container_;
};

// Test basic container operations
TEST_F(SoAContainerTest, BasicOperations) {
    // Test empty container
    EXPECT_EQ(container_->size(), 0u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 0u);
    
    // Add shapes
    Circle circle(10.0f, 20.0f, 5.0f);
    Rectangle rect(0.0f, 0.0f, 30.0f, 40.0f);
    Ellipse ellipse(15.0f, 25.0f, 10.0f, 8.0f);
    Line line(0.0f, 0.0f, 50.0f, 50.0f);
    
    uint32_t circle_id = container_->add_circle(circle);
    uint32_t rect_id = container_->add_rectangle(rect);
    uint32_t ellipse_id = container_->add_ellipse(ellipse);
    uint32_t line_id = container_->add_line(line);
    
    // Verify counts
    EXPECT_EQ(container_->size(), 4u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 1u);
    EXPECT_EQ(container_->count(ShapeType::Rectangle), 1u);
    EXPECT_EQ(container_->count(ShapeType::Ellipse), 1u);
    EXPECT_EQ(container_->count(ShapeType::Line), 1u);
    
    // Verify IDs are unique
    EXPECT_NE(circle_id, rect_id);
    EXPECT_NE(circle_id, ellipse_id);
    EXPECT_NE(circle_id, line_id);
    EXPECT_NE(rect_id, ellipse_id);
    EXPECT_NE(rect_id, line_id);
    EXPECT_NE(ellipse_id, line_id);
    
    // Test contains
    EXPECT_TRUE(container_->contains(circle_id));
    EXPECT_TRUE(container_->contains(rect_id));
    EXPECT_FALSE(container_->contains(9999));
}

// Test shape retrieval
TEST_F(SoAContainerTest, ShapeRetrieval) {
    // Add shapes
    Circle circle(10.0f, 20.0f, 5.0f);
    uint32_t circle_id = container_->add_circle(circle);
    
    Rectangle rect(5.0f, 10.0f, 25.0f, 35.0f);
    uint32_t rect_id = container_->add_rectangle(rect);
    
    // Retrieve shapes
    CircleShape* retrieved_circle = container_->get<CircleShape>(circle_id);
    ASSERT_NE(retrieved_circle, nullptr);
    EXPECT_FLOAT_EQ(retrieved_circle->center_x, 10.0f);
    EXPECT_FLOAT_EQ(retrieved_circle->center_y, 20.0f);
    EXPECT_FLOAT_EQ(retrieved_circle->radius, 5.0f);
    
    RectangleShape* retrieved_rect = container_->get<RectangleShape>(rect_id);
    ASSERT_NE(retrieved_rect, nullptr);
    EXPECT_FLOAT_EQ(retrieved_rect->x1, 5.0f);
    EXPECT_FLOAT_EQ(retrieved_rect->y1, 10.0f);
    EXPECT_FLOAT_EQ(retrieved_rect->x2, 30.0f);  // 5 + 25
    EXPECT_FLOAT_EQ(retrieved_rect->y2, 45.0f);  // 10 + 35
    
    // Test wrong type retrieval
    CircleShape* wrong_type = container_->get<CircleShape>(rect_id);
    EXPECT_EQ(wrong_type, nullptr);
    
    // Test non-existent ID
    CircleShape* non_existent = container_->get<CircleShape>(9999);
    EXPECT_EQ(non_existent, nullptr);
}

// Test shape removal
TEST_F(SoAContainerTest, ShapeRemoval) {
    // Add multiple shapes
    std::vector<uint32_t> circle_ids;
    for (int i = 0; i < 5; ++i) {
        Circle c(i * 10.0f, i * 20.0f, 5.0f);
        circle_ids.push_back(container_->add_circle(c));
    }
    
    EXPECT_EQ(container_->size(), 5u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 5u);
    
    // Remove middle shape
    EXPECT_TRUE(container_->remove(circle_ids[2]));
    EXPECT_EQ(container_->size(), 4u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 4u);
    EXPECT_FALSE(container_->contains(circle_ids[2]));
    
    // Try to remove again
    EXPECT_FALSE(container_->remove(circle_ids[2]));
    
    // Remove all remaining
    for (size_t i = 0; i < circle_ids.size(); ++i) {
        if (i != 2) {  // Skip already removed
            EXPECT_TRUE(container_->remove(circle_ids[i]));
        }
    }
    
    EXPECT_EQ(container_->size(), 0u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 0u);
}

// Test memory layout and alignment
TEST_F(SoAContainerTest, MemoryLayoutAlignment) {
    // Add many shapes to test array layout
    const size_t count = 100;
    
    for (size_t i = 0; i < count; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        container_->add_circle(c);
    }
    
    // Get direct access to arrays
    const auto& circles = container_->get_circles();
    EXPECT_GE(circles.capacity(), count);
    
    // Verify contiguous memory
    if (circles.size() >= 2) {
        const CircleShape* first = &circles[0];
        const CircleShape* second = &circles[1];
        ptrdiff_t diff = reinterpret_cast<const char*>(second) - 
                        reinterpret_cast<const char*>(first);
        EXPECT_EQ(diff, sizeof(CircleShape));
    }
    
    // Test alignment (should be suitable for SIMD)
    const void* data_ptr = circles.data();
    uintptr_t addr = reinterpret_cast<uintptr_t>(data_ptr);
    EXPECT_EQ(addr % alignof(CircleShape), 0u);
}

// Test iteration methods
TEST_F(SoAContainerTest, IterationMethods) {
    // Add shapes of different types
    std::vector<uint32_t> ids;
    
    for (int i = 0; i < 3; ++i) {
        Circle c(i * 10.0f, i * 10.0f, 5.0f);
        ids.push_back(container_->add_circle(c));
    }
    
    for (int i = 0; i < 2; ++i) {
        Rectangle r(i * 20.0f, i * 20.0f, 10.0f, 10.0f);
        ids.push_back(container_->add_rectangle(r));
    }
    
    // Test type-specific iteration
    int circle_count = 0;
    container_->for_each_type<CircleShape>([&circle_count](CircleShape& circle) {
        circle_count++;
        EXPECT_FLOAT_EQ(circle.radius, 5.0f);
    });
    EXPECT_EQ(circle_count, 3);
    
    int rect_count = 0;
    container_->for_each_type<RectangleShape>([&rect_count](RectangleShape& rect) {
        rect_count++;
        EXPECT_FLOAT_EQ(rect.x2 - rect.x1, 10.0f);  // Width
    });
    EXPECT_EQ(rect_count, 2);
    
    // Test sorted iteration
    int total_count = 0;
    container_->for_each_sorted([&total_count](const auto& shape) {
        (void)shape;  // Unused but required by template
        total_count++;
    });
    EXPECT_EQ(total_count, 5);
}

// Test z-index ordering
TEST_F(SoAContainerTest, ZIndexOrdering) {
    // Add shapes with explicit z-indices
    std::vector<uint32_t> ids;
    std::vector<uint16_t> z_indices = {5, 1, 3, 2, 4};
    
    for (size_t i = 0; i < z_indices.size(); ++i) {
        Circle c(i * 10.0f, 0.0f, 5.0f);
        uint32_t id = container_->add_circle(c);
        container_->set_z_index(id, z_indices[i]);
        ids.push_back(id);
    }
    
    // Verify sorted iteration order
    std::vector<float> x_coords;
    container_->for_each_sorted([&x_coords](const auto& shape) {
        // We know all shapes in this test are circles
        using ShapeType = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            x_coords.push_back(shape.get_center_x());
        }
    });
    
    // Should be sorted by z-index: 1,2,3,4,5 -> indices 1,3,2,4,0
    ASSERT_EQ(x_coords.size(), 5u);
    EXPECT_FLOAT_EQ(x_coords[0], 10.0f);  // z=1, original index 1
    EXPECT_FLOAT_EQ(x_coords[1], 30.0f);  // z=2, original index 3
    EXPECT_FLOAT_EQ(x_coords[2], 20.0f);  // z=3, original index 2
    EXPECT_FLOAT_EQ(x_coords[3], 40.0f);  // z=4, original index 4
    EXPECT_FLOAT_EQ(x_coords[4], 0.0f);   // z=5, original index 0
}

// Test visibility
TEST_F(SoAContainerTest, VisibilityControl) {
    // Add shapes
    Circle c1(10.0f, 10.0f, 5.0f);
    Circle c2(20.0f, 20.0f, 5.0f);
    Circle c3(30.0f, 30.0f, 5.0f);
    
    uint32_t id1 = container_->add_circle(c1);
    uint32_t id2 = container_->add_circle(c2);
    uint32_t id3 = container_->add_circle(c3);
    
    // Hide middle shape
    EXPECT_TRUE(container_->set_visible(id2, false));
    
    // Count visible shapes in iteration
    int visible_count = 0;
    container_->for_each_sorted([&visible_count](const auto& shape) {
        (void)shape;  // Unused but required by template
        visible_count++;
    });
    
    EXPECT_EQ(visible_count, 2);  // Only 2 visible
    
    // Make visible again
    EXPECT_TRUE(container_->set_visible(id2, true));
    
    visible_count = 0;
    container_->for_each_sorted([&visible_count](const auto& shape) {
        (void)shape;  // Unused but required by template
        visible_count++;
    });
    
    EXPECT_EQ(visible_count, 3);  // All visible
}

// Test bounds calculation
TEST_F(SoAContainerTest, BoundsCalculation) {
    // Empty container
    BoundingBox empty_bounds = container_->get_bounds();
    EXPECT_FLOAT_EQ(empty_bounds.min_x, 0.0f);
    EXPECT_FLOAT_EQ(empty_bounds.min_y, 0.0f);
    EXPECT_FLOAT_EQ(empty_bounds.max_x, 0.0f);
    EXPECT_FLOAT_EQ(empty_bounds.max_y, 0.0f);
    
    // Add shapes
    Circle c1(10.0f, 20.0f, 5.0f);  // Bounds: (5,15) to (15,25)
    Rectangle r1(0.0f, 0.0f, 30.0f, 10.0f);  // Bounds: (0,0) to (30,10)
    Line l1(50.0f, 5.0f, 60.0f, 15.0f);  // Bounds: (50,5) to (60,15)
    
    container_->add_circle(c1);
    container_->add_rectangle(r1);
    container_->add_line(l1);
    
    BoundingBox bounds = container_->get_bounds();
    EXPECT_FLOAT_EQ(bounds.min_x, 0.0f);   // From rectangle
    EXPECT_FLOAT_EQ(bounds.min_y, 0.0f);   // From rectangle
    EXPECT_FLOAT_EQ(bounds.max_x, 60.0f);  // From line
    EXPECT_FLOAT_EQ(bounds.max_y, 25.0f);  // From circle
    
    // Test bounds caching
    BoundingBox bounds2 = container_->get_bounds();
    EXPECT_EQ(bounds.min_x, bounds2.min_x);
    EXPECT_EQ(bounds.min_y, bounds2.min_y);
    EXPECT_EQ(bounds.max_x, bounds2.max_x);
    EXPECT_EQ(bounds.max_y, bounds2.max_y);
}

// Test bounds with visibility
TEST_F(SoAContainerTest, BoundsWithVisibility) {
    // Add shapes with one extending the bounds
    Circle c1(10.0f, 10.0f, 5.0f);  // Bounds: (5,5) to (15,15)
    Circle c2(100.0f, 100.0f, 10.0f);  // Bounds: (90,90) to (110,110) - extends bounds
    
    uint32_t id1 = container_->add_circle(c1);
    uint32_t id2 = container_->add_circle(c2);
    
    // Initial bounds
    BoundingBox bounds1 = container_->get_bounds();
    EXPECT_FLOAT_EQ(bounds1.min_x, 5.0f);
    EXPECT_FLOAT_EQ(bounds1.max_x, 110.0f);
    
    // Hide the shape that extends bounds
    container_->set_visible(id2, false);
    
    // Bounds should shrink
    BoundingBox bounds2 = container_->get_bounds();
    EXPECT_FLOAT_EQ(bounds2.min_x, 5.0f);
    EXPECT_FLOAT_EQ(bounds2.max_x, 15.0f);
    
    // All shapes hidden
    container_->set_visible(id1, false);
    BoundingBox bounds3 = container_->get_bounds();
    EXPECT_FLOAT_EQ(bounds3.min_x, 0.0f);
    EXPECT_FLOAT_EQ(bounds3.max_x, 0.0f);
}

// Test memory compaction
TEST_F(SoAContainerTest, MemoryCompaction) {
    // Add many shapes
    std::vector<uint32_t> ids;
    for (int i = 0; i < 10; ++i) {
        Circle c(i * 10.0f, 0.0f, 5.0f);
        ids.push_back(container_->add_circle(c));
    }
    
    // Remove every other shape
    for (size_t i = 0; i < ids.size(); i += 2) {
        container_->remove(ids[i]);
    }
    
    EXPECT_EQ(container_->size(), 5u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 5u);
    
    // Compact memory
    container_->compact();
    
    // Verify remaining shapes are still accessible
    for (size_t i = 1; i < ids.size(); i += 2) {
        CircleShape* circle = container_->get<CircleShape>(ids[i]);
        ASSERT_NE(circle, nullptr);
        EXPECT_FLOAT_EQ(circle->center_x, i * 10.0f);
    }
}

// Test reserve capacity
TEST_F(SoAContainerTest, ReserveCapacity) {
    const size_t reserve_size = 10000;
    container_->reserve(reserve_size);
    
    // Add many shapes efficiently
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < reserve_size / 4; ++i) {
        Circle c(i * 1.0f, i * 1.0f, 1.0f);
        Rectangle r(i * 2.0f, i * 2.0f, 1.0f, 1.0f);
        Ellipse e(i * 3.0f, i * 3.0f, 1.0f, 1.0f);
        Line l(i * 4.0f, i * 4.0f, i * 5.0f, i * 5.0f);
        
        container_->add_circle(c);
        container_->add_rectangle(r);
        container_->add_ellipse(e);
        container_->add_line(l);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_EQ(container_->size(), reserve_size);
    
    // Should be fast due to pre-allocation
    double us_per_shape = static_cast<double>(duration.count()) / reserve_size;
    std::cout << "Shape insertion with reserve: " << us_per_shape << " μs/shape\n";
}

// Test clear operation
TEST_F(SoAContainerTest, ClearContainer) {
    // Add shapes
    for (int i = 0; i < 100; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        container_->add_circle(c);
    }
    
    EXPECT_EQ(container_->size(), 100u);
    EXPECT_NE(container_->get_bounds().max_x, 0.0f);
    
    // Clear
    container_->clear();
    
    EXPECT_EQ(container_->size(), 0u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 0u);
    
    BoundingBox bounds = container_->get_bounds();
    EXPECT_FLOAT_EQ(bounds.min_x, 0.0f);
    EXPECT_FLOAT_EQ(bounds.max_x, 0.0f);
    
    // Can add shapes again
    Circle c(10.0f, 20.0f, 5.0f);
    uint32_t id = container_->add_circle(c);
    EXPECT_GT(id, 0u);
    EXPECT_EQ(container_->size(), 1u);
}

// Test SIMD-friendly access patterns
TEST_F(SoAContainerTest, SimdAccessPatterns) {
    // Add aligned number of shapes for SIMD
    const size_t count = 256;  // Multiple of common SIMD widths
    
    for (size_t i = 0; i < count; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        container_->add_circle(c);
    }
    
    // Test batch transformation using direct array access
    const auto& circles = container_->get_circles();
    EXPECT_EQ(circles.size(), count);
    
    // Simulate SIMD batch operation
    std::vector<float> x_sum(count / 8, 0.0f);  // AVX processes 8 floats
    
    for (size_t i = 0; i < count; i += 8) {
        // In real SIMD, this would be a single instruction
        float sum = 0.0f;
        for (size_t j = 0; j < 8 && (i + j) < count; ++j) {
            sum += circles[i + j].center_x;
        }
        x_sum[i / 8] = sum;
    }
    
    // Verify results
    float expected_sum = 0.0f;
    for (size_t i = 0; i < 8; ++i) {
        expected_sum += i * 1.0f;
    }
    EXPECT_FLOAT_EQ(x_sum[0], expected_sum);
}

// Performance comparison test
TEST_F(SoAContainerTest, PerformanceComparison) {
    const size_t shape_count = 10000;
    
    // Time bulk insertion
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < shape_count; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 5.0f);
        container_->add_circle(c);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Time iteration
    start = std::chrono::high_resolution_clock::now();
    
    float sum = 0.0f;
    container_->for_each_type<CircleShape>([&sum](const CircleShape& circle) {
        sum += circle.center_x + circle.center_y + circle.radius;
    });
    
    end = std::chrono::high_resolution_clock::now();
    auto iter_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Time bounds calculation
    start = std::chrono::high_resolution_clock::now();
    BoundingBox bounds = container_->get_bounds();
    end = std::chrono::high_resolution_clock::now();
    auto bounds_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "SoA Container Performance:\n";
    std::cout << "  Insertion: " << insert_duration.count() / 1000.0 << " ms for " 
              << shape_count << " shapes (" 
              << static_cast<double>(insert_duration.count()) / shape_count << " μs/shape)\n";
    std::cout << "  Iteration: " << iter_duration.count() / 1000.0 << " ms ("
              << static_cast<double>(iter_duration.count()) / shape_count << " μs/shape)\n";
    std::cout << "  Bounds: " << bounds_duration.count() << " μs\n";
    
    // Verify results
    EXPECT_GT(sum, 0.0f);
    EXPECT_GT(bounds.max_x, bounds.min_x);
}