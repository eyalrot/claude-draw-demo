#include <gtest/gtest.h>
#include "claude_draw/shapes/circle_optimized.h"
#include <chrono>
#include <vector>
#include <random>

using namespace claude_draw::shapes;
using namespace claude_draw;

class CircleOptimizedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed
    }
};

// Test basic circle creation
TEST_F(CircleOptimizedTest, BasicConstruction) {
    // Default constructor
    Circle c1;
    EXPECT_EQ(c1.get_center_x(), 0.0f);
    EXPECT_EQ(c1.get_center_y(), 0.0f);
    EXPECT_EQ(c1.get_radius(), 0.0f);
    
    // Constructor with parameters
    Circle c2(10.0f, 20.0f, 5.0f);
    EXPECT_EQ(c2.get_center_x(), 10.0f);
    EXPECT_EQ(c2.get_center_y(), 20.0f);
    EXPECT_EQ(c2.get_radius(), 5.0f);
    
    // Constructor with Point2D
    Point2D center(15.0f, 25.0f);
    Circle c3(center, 7.0f);
    EXPECT_EQ(c3.get_center_x(), 15.0f);
    EXPECT_EQ(c3.get_center_y(), 25.0f);
    EXPECT_EQ(c3.get_radius(), 7.0f);
}

// Test memory footprint
TEST_F(CircleOptimizedTest, MemoryFootprint) {
    // Circle should have minimal overhead over CircleShape
    EXPECT_EQ(sizeof(Circle), sizeof(CircleShape));
    EXPECT_EQ(sizeof(Circle), 32u);
}

// Test bounding box calculation
TEST_F(CircleOptimizedTest, BoundingBox) {
    Circle circle(10.0f, 20.0f, 5.0f);
    BoundingBox bounds = circle.get_bounds();
    
    EXPECT_FLOAT_EQ(bounds.min_x, 5.0f);
    EXPECT_FLOAT_EQ(bounds.min_y, 15.0f);
    EXPECT_FLOAT_EQ(bounds.max_x, 15.0f);
    EXPECT_FLOAT_EQ(bounds.max_y, 25.0f);
    
    // Test with zero radius
    Circle point(0.0f, 0.0f, 0.0f);
    BoundingBox point_bounds = point.get_bounds();
    EXPECT_FLOAT_EQ(point_bounds.min_x, 0.0f);
    EXPECT_FLOAT_EQ(point_bounds.min_y, 0.0f);
    EXPECT_FLOAT_EQ(point_bounds.max_x, 0.0f);
    EXPECT_FLOAT_EQ(point_bounds.max_y, 0.0f);
}

// Test point containment
TEST_F(CircleOptimizedTest, PointContainment) {
    Circle circle(0.0f, 0.0f, 10.0f);
    
    // Points inside
    EXPECT_TRUE(circle.contains_point(0.0f, 0.0f));  // Center
    EXPECT_TRUE(circle.contains_point(5.0f, 0.0f));  // Right of center
    EXPECT_TRUE(circle.contains_point(0.0f, -5.0f)); // Below center
    EXPECT_TRUE(circle.contains_point(7.0f, 7.0f));  // Diagonal (within)
    
    // Points on perimeter (should be included)
    EXPECT_TRUE(circle.contains_point(10.0f, 0.0f));
    EXPECT_TRUE(circle.contains_point(0.0f, 10.0f));
    EXPECT_TRUE(circle.contains_point(-10.0f, 0.0f));
    EXPECT_TRUE(circle.contains_point(0.0f, -10.0f));
    
    // Points outside
    EXPECT_FALSE(circle.contains_point(11.0f, 0.0f));
    EXPECT_FALSE(circle.contains_point(8.0f, 8.0f));  // Diagonal (outside)
    EXPECT_FALSE(circle.contains_point(15.0f, 15.0f));
    
    // Test with Point2D
    EXPECT_TRUE(circle.contains_point(Point2D(5.0f, 5.0f)));
    EXPECT_FALSE(circle.contains_point(Point2D(15.0f, 0.0f)));
}

// Test geometric calculations
TEST_F(CircleOptimizedTest, GeometricCalculations) {
    Circle circle(0.0f, 0.0f, 10.0f);
    
    // Circumference = 2 * pi * r
    EXPECT_FLOAT_EQ(circle.circumference(), 2.0f * M_PI * 10.0f);
    
    // Area = pi * r^2
    EXPECT_FLOAT_EQ(circle.area(), M_PI * 100.0f);
    
    // Test with different radius
    circle.set_radius(5.0f);
    EXPECT_FLOAT_EQ(circle.circumference(), 2.0f * M_PI * 5.0f);
    EXPECT_FLOAT_EQ(circle.area(), M_PI * 25.0f);
}

// Test transformations
TEST_F(CircleOptimizedTest, Transformations) {
    Circle circle(10.0f, 20.0f, 5.0f);
    
    // Translation
    Transform2D translate = Transform2D::translate(5.0f, -10.0f);
    circle.transform(translate);
    EXPECT_FLOAT_EQ(circle.get_center_x(), 15.0f);
    EXPECT_FLOAT_EQ(circle.get_center_y(), 10.0f);
    EXPECT_FLOAT_EQ(circle.get_radius(), 5.0f);  // Radius unchanged
    
    // Uniform scaling
    Transform2D scale = Transform2D::scale(2.0f, 2.0f);
    circle.transform(scale);
    EXPECT_FLOAT_EQ(circle.get_center_x(), 30.0f);
    EXPECT_FLOAT_EQ(circle.get_center_y(), 20.0f);
    EXPECT_FLOAT_EQ(circle.get_radius(), 10.0f);  // Radius scaled
    
    // Rotation (center should rotate, radius unchanged)
    Circle circle2(10.0f, 0.0f, 5.0f);
    Transform2D rotate = Transform2D::rotate(M_PI / 2);  // 90 degrees
    circle2.transform(rotate);
    EXPECT_NEAR(circle2.get_center_x(), 0.0f, 1e-6f);
    EXPECT_NEAR(circle2.get_center_y(), 10.0f, 1e-6f);
    EXPECT_FLOAT_EQ(circle2.get_radius(), 5.0f);
}

// Test circle-circle intersection
TEST_F(CircleOptimizedTest, CircleIntersection) {
    Circle c1(0.0f, 0.0f, 10.0f);
    
    // Overlapping circles
    Circle c2(15.0f, 0.0f, 10.0f);  // Centers 15 apart, radii sum = 20
    EXPECT_TRUE(c1.intersects_circle(c2));
    
    // Touching circles
    Circle c3(20.0f, 0.0f, 10.0f);  // Centers 20 apart, radii sum = 20
    EXPECT_TRUE(c1.intersects_circle(c3));
    
    // Non-intersecting circles
    Circle c4(25.0f, 0.0f, 10.0f);  // Centers 25 apart, radii sum = 20
    EXPECT_FALSE(c1.intersects_circle(c4));
    
    // Concentric circles
    Circle c5(0.0f, 0.0f, 5.0f);
    EXPECT_TRUE(c1.intersects_circle(c5));
    
    // Same circle
    EXPECT_TRUE(c1.intersects_circle(c1));
}

// Test style management
TEST_F(CircleOptimizedTest, StyleManagement) {
    Circle circle(0.0f, 0.0f, 10.0f);
    
    // Default colors
    Color default_fill = circle.get_fill_color();
    EXPECT_EQ(default_fill.r, 0);
    EXPECT_EQ(default_fill.g, 0);
    EXPECT_EQ(default_fill.b, 0);
    EXPECT_EQ(default_fill.a, 255);
    
    // Set fill color
    Color red(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    circle.set_fill_color(red);
    Color fill = circle.get_fill_color();
    EXPECT_EQ(fill.r, 255);
    EXPECT_EQ(fill.g, 0);
    EXPECT_EQ(fill.b, 0);
    EXPECT_EQ(fill.a, 255);
    
    // Set stroke color
    Color blue(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(128));
    circle.set_stroke_color(blue);
    Color stroke = circle.get_stroke_color();
    EXPECT_EQ(stroke.r, 0);
    EXPECT_EQ(stroke.g, 0);
    EXPECT_EQ(stroke.b, 255);
    EXPECT_EQ(stroke.a, 128);
    
    // Stroke width
    EXPECT_FLOAT_EQ(circle.get_stroke_width(), 1.0f);  // Default
    circle.set_stroke_width(2.5f);
    EXPECT_FLOAT_EQ(circle.get_stroke_width(), 2.5f);
}

// Test flags and visibility
TEST_F(CircleOptimizedTest, FlagsAndVisibility) {
    Circle circle(0.0f, 0.0f, 10.0f);
    
    // Default flags
    EXPECT_TRUE(circle.is_visible());
    EXPECT_TRUE(circle.is_filled());
    EXPECT_FALSE(circle.is_stroked());
    
    // Toggle visibility
    circle.set_visible(false);
    EXPECT_FALSE(circle.is_visible());
    circle.set_visible(true);
    EXPECT_TRUE(circle.is_visible());
    
    // Toggle fill
    circle.set_filled(false);
    EXPECT_FALSE(circle.is_filled());
    circle.set_filled(true);
    EXPECT_TRUE(circle.is_filled());
    
    // Toggle stroke
    circle.set_stroked(true);
    EXPECT_TRUE(circle.is_stroked());
    circle.set_stroked(false);
    EXPECT_FALSE(circle.is_stroked());
}

// Test edge cases
TEST_F(CircleOptimizedTest, EdgeCases) {
    // Zero radius circle
    Circle zero_circle(10.0f, 20.0f, 0.0f);
    EXPECT_FLOAT_EQ(zero_circle.area(), 0.0f);
    EXPECT_FLOAT_EQ(zero_circle.circumference(), 0.0f);
    EXPECT_TRUE(zero_circle.contains_point(10.0f, 20.0f));  // Only contains center
    EXPECT_FALSE(zero_circle.contains_point(10.1f, 20.0f));
    
    // Negative radius (should work with absolute value in calculations)
    Circle neg_circle(0.0f, 0.0f, -5.0f);
    // Most calculations use radius squared, so sign doesn't matter
    EXPECT_TRUE(neg_circle.contains_point(3.0f, 0.0f));
    
    // Very large circle
    Circle large_circle(0.0f, 0.0f, 1e6f);
    EXPECT_TRUE(large_circle.contains_point(999999.0f, 0.0f));
    EXPECT_FALSE(large_circle.contains_point(1000001.0f, 0.0f));
}

// Test batch operations
TEST_F(CircleOptimizedTest, BatchOperations) {
    const size_t count = 1000;
    std::vector<Circle> circles;
    std::vector<Point2D> points;

    
    // Create test data
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
    
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(dist(rng), dist(rng), std::abs(dist(rng)) * 0.5f);
        points.emplace_back(dist(rng), dist(rng));
    }
    
    // Test batch contains
    std::unique_ptr<bool[]> bool_results(new bool[count]);
    Circle::batch_contains(circles.data(), count, points.data(), bool_results.get());
    
    // Verify a few results manually
    for (size_t i = 0; i < 10; ++i) {
        bool expected = circles[i].contains_point(points[i]);
        EXPECT_EQ(bool_results[i], expected);
    }
    
    // Test batch transform
    Transform2D transform = Transform2D::translate(10.0f, 20.0f);
    std::vector<Circle> circles_copy = circles;
    Circle::batch_transform(circles_copy.data(), count, transform);
    
    // Verify transforms
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(circles_copy[i].get_center_x(), circles[i].get_center_x() + 10.0f);
        EXPECT_FLOAT_EQ(circles_copy[i].get_center_y(), circles[i].get_center_y() + 20.0f);
    }
}

// Performance test - circle creation
TEST_F(CircleOptimizedTest, CreationPerformance) {
    const size_t iterations = 1000000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        volatile Circle c(i % 100, i % 200, i % 50 + 1);
        (void)c;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double ns_per_creation = static_cast<double>(duration.count()) / iterations;
    
    // Should be sub-microsecond (< 1000 nanoseconds)
    EXPECT_LT(ns_per_creation, 1000.0);
    
    // Should ideally be < 500 nanoseconds as per requirements
    if (ns_per_creation < 500.0) {
        std::cout << "Circle creation performance: EXCELLENT - " 
                  << ns_per_creation << " ns/op" << std::endl;
    } else {
        std::cout << "Circle creation performance: " 
                  << ns_per_creation << " ns/op" << std::endl;
    }
}

// Test distance to edge
TEST_F(CircleOptimizedTest, DistanceToEdge) {
    Circle circle(0.0f, 0.0f, 10.0f);
    
    // Point at center
    EXPECT_FLOAT_EQ(circle.distance_to_edge(Point2D(0.0f, 0.0f)), 10.0f);
    
    // Point on edge
    EXPECT_NEAR(circle.distance_to_edge(Point2D(10.0f, 0.0f)), 0.0f, 1e-6f);
    
    // Point inside
    EXPECT_FLOAT_EQ(circle.distance_to_edge(Point2D(5.0f, 0.0f)), 5.0f);
    
    // Point outside
    EXPECT_FLOAT_EQ(circle.distance_to_edge(Point2D(15.0f, 0.0f)), 5.0f);
}

// Test clone operation
TEST_F(CircleOptimizedTest, CloneOperation) {
    Circle original(10.0f, 20.0f, 5.0f);
    original.set_fill_color(Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
    original.set_stroke_width(3.0f);
    original.set_id(42);
    
    auto cloned = original.clone();
    
    EXPECT_EQ(cloned->get_center_x(), original.get_center_x());
    EXPECT_EQ(cloned->get_center_y(), original.get_center_y());
    EXPECT_EQ(cloned->get_radius(), original.get_radius());
    EXPECT_EQ(cloned->get_id(), original.get_id());
    
    // Modify clone shouldn't affect original
    cloned->set_center(0.0f, 0.0f);
    EXPECT_EQ(original.get_center_x(), 10.0f);
    EXPECT_EQ(original.get_center_y(), 20.0f);
}