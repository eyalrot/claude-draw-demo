#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "claude_draw/shapes/circle.h"
#include "claude_draw/core/transform2d.h"
#include <cmath>

using namespace claude_draw;
using namespace claude_draw::shapes;

class CircleTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test default construction
TEST_F(CircleTest, DefaultConstruction) {
    Circle circle;
    
    EXPECT_EQ(circle.get_center().x, 0.0f);
    EXPECT_EQ(circle.get_center().y, 0.0f);
    EXPECT_EQ(circle.get_radius(), 0.0f);
    EXPECT_EQ(circle.get_radius_squared(), 0.0f);
    EXPECT_EQ(circle.get_type(), ShapeType::Circle);
}

// Test parameterized construction
TEST_F(CircleTest, ParameterizedConstruction) {
    // With Point2D center
    Circle circle1(Point2D(10.0f, 20.0f), 5.0f);
    EXPECT_EQ(circle1.get_center().x, 10.0f);
    EXPECT_EQ(circle1.get_center().y, 20.0f);
    EXPECT_EQ(circle1.get_radius(), 5.0f);
    EXPECT_EQ(circle1.get_radius_squared(), 25.0f);
    
    // With separate coordinates
    Circle circle2(15.0f, 25.0f, 7.0f);
    EXPECT_EQ(circle2.get_center().x, 15.0f);
    EXPECT_EQ(circle2.get_center().y, 25.0f);
    EXPECT_EQ(circle2.get_radius(), 7.0f);
    EXPECT_EQ(circle2.get_radius_squared(), 49.0f);
}

// Test setters
TEST_F(CircleTest, Setters) {
    Circle circle;
    
    // Set center with Point2D
    circle.set_center(Point2D(5.0f, 10.0f));
    EXPECT_EQ(circle.get_center().x, 5.0f);
    EXPECT_EQ(circle.get_center().y, 10.0f);
    
    // Set center with coordinates
    circle.set_center(15.0f, 20.0f);
    EXPECT_EQ(circle.get_center().x, 15.0f);
    EXPECT_EQ(circle.get_center().y, 20.0f);
    
    // Set radius
    circle.set_radius(8.0f);
    EXPECT_EQ(circle.get_radius(), 8.0f);
    EXPECT_EQ(circle.get_radius_squared(), 64.0f);
}

// Test bounding box calculation
TEST_F(CircleTest, BoundingBox) {
    Circle circle(10.0f, 20.0f, 5.0f);
    BoundingBox bounds = circle.calculate_bounds();
    
    EXPECT_EQ(bounds.min_x, 5.0f);
    EXPECT_EQ(bounds.min_y, 15.0f);
    EXPECT_EQ(bounds.max_x, 15.0f);
    EXPECT_EQ(bounds.max_y, 25.0f);
    
    // Test cached bounds
    const BoundingBox& cached_bounds = circle.get_bounds();
    EXPECT_EQ(cached_bounds.min_x, 5.0f);
    EXPECT_EQ(cached_bounds.min_y, 15.0f);
    EXPECT_EQ(cached_bounds.max_x, 15.0f);
    EXPECT_EQ(cached_bounds.max_y, 25.0f);
}

// Test point containment
TEST_F(CircleTest, ContainsPoint) {
    Circle circle(10.0f, 10.0f, 5.0f);
    
    // Center point
    EXPECT_TRUE(circle.contains_point(Point2D(10.0f, 10.0f)));
    
    // Points on the circumference
    EXPECT_TRUE(circle.contains_point(Point2D(15.0f, 10.0f)));
    EXPECT_TRUE(circle.contains_point(Point2D(10.0f, 15.0f)));
    EXPECT_TRUE(circle.contains_point(Point2D(5.0f, 10.0f)));
    EXPECT_TRUE(circle.contains_point(Point2D(10.0f, 5.0f)));
    
    // Points inside
    EXPECT_TRUE(circle.contains_point(Point2D(12.0f, 12.0f)));
    EXPECT_TRUE(circle.contains_point(Point2D(8.0f, 8.0f)));
    
    // Points outside
    EXPECT_FALSE(circle.contains_point(Point2D(16.0f, 10.0f)));
    EXPECT_FALSE(circle.contains_point(Point2D(10.0f, 16.0f)));
    EXPECT_FALSE(circle.contains_point(Point2D(4.0f, 10.0f)));
    EXPECT_FALSE(circle.contains_point(Point2D(10.0f, 4.0f)));
    
    // Test with bounds_contains fast path
    EXPECT_TRUE(circle.bounds_contains(Point2D(10.0f, 10.0f)));
    EXPECT_FALSE(circle.bounds_contains(Point2D(20.0f, 20.0f)));
}

// Test transformations
TEST_F(CircleTest, Transformations) {
    Circle circle(10.0f, 10.0f, 5.0f);
    
    // Translation
    Transform2D translate = Transform2D::translate(5.0f, -3.0f);
    circle.transform(translate);
    EXPECT_FLOAT_EQ(circle.get_center().x, 15.0f);
    EXPECT_FLOAT_EQ(circle.get_center().y, 7.0f);
    EXPECT_FLOAT_EQ(circle.get_radius(), 5.0f);  // Radius unchanged
    
    // Uniform scaling
    Transform2D scale = Transform2D::scale(2.0f, 2.0f);
    circle.transform(scale);
    EXPECT_FLOAT_EQ(circle.get_center().x, 30.0f);
    EXPECT_FLOAT_EQ(circle.get_center().y, 14.0f);
    EXPECT_FLOAT_EQ(circle.get_radius(), 10.0f);  // Radius scaled
    
    // Rotation (center moves, radius unchanged)
    Circle circle2(10.0f, 0.0f, 5.0f);
    Transform2D rotate = Transform2D::rotate(M_PI / 2);  // 90 degrees
    circle2.transform(rotate);
    EXPECT_NEAR(circle2.get_center().x, 0.0f, 1e-5);
    EXPECT_NEAR(circle2.get_center().y, 10.0f, 1e-5);
    EXPECT_FLOAT_EQ(circle2.get_radius(), 5.0f);
}

// Test geometric calculations
TEST_F(CircleTest, GeometricCalculations) {
    Circle circle(0.0f, 0.0f, 10.0f);
    
    // Circumference
    EXPECT_FLOAT_EQ(circle.circumference(), 2.0f * M_PI * 10.0f);
    
    // Area
    EXPECT_FLOAT_EQ(circle.area(), M_PI * 100.0f);
}

// Test circle intersection
TEST_F(CircleTest, CircleIntersection) {
    Circle circle1(0.0f, 0.0f, 5.0f);
    Circle circle2(8.0f, 0.0f, 5.0f);
    Circle circle3(20.0f, 0.0f, 5.0f);
    
    // Intersecting circles
    EXPECT_TRUE(circle1.intersects_circle(circle2));
    EXPECT_TRUE(circle2.intersects_circle(circle1));
    
    // Non-intersecting circles
    EXPECT_FALSE(circle1.intersects_circle(circle3));
    EXPECT_FALSE(circle3.intersects_circle(circle1));
    
    // Touching circles (edge case)
    Circle circle4(10.0f, 0.0f, 5.0f);
    EXPECT_TRUE(circle1.intersects_circle(circle4));
    
    // Concentric circles
    Circle circle5(0.0f, 0.0f, 3.0f);
    EXPECT_TRUE(circle1.intersects_circle(circle5));
}

// Test cloning
TEST_F(CircleTest, Cloning) {
    Circle original(15.0f, 25.0f, 7.5f);
    original.set_style(ShapeStyle(Color(uint8_t(255), uint8_t(0), uint8_t(0)), Color(uint8_t(0), uint8_t(255), uint8_t(0)), 2.0f));
    
    auto cloned = original.clone();
    ASSERT_NE(cloned, nullptr);
    
    Circle* cloned_circle = dynamic_cast<Circle*>(cloned.get());
    ASSERT_NE(cloned_circle, nullptr);
    
    EXPECT_EQ(cloned_circle->get_center().x, 15.0f);
    EXPECT_EQ(cloned_circle->get_center().y, 25.0f);
    EXPECT_EQ(cloned_circle->get_radius(), 7.5f);
    EXPECT_EQ(cloned_circle->get_style().fill_color.rgba, Color(uint8_t(255), uint8_t(0), uint8_t(0)).rgba);
}

// Test batch operations
TEST_F(CircleTest, BatchOperations) {
    const size_t count = 100;
    std::vector<Circle> circles;
    circles.reserve(count);
    
    // Create circles
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(i * 10.0f, i * 5.0f, 3.0f);
    }
    
    // Batch transform
    Transform2D transform = Transform2D::scale(2.0f, 2.0f);
    Circle::batch_transform(circles.data(), count, transform);
    
    // Verify transformation
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(circles[i].get_center().x, i * 20.0f);
        EXPECT_FLOAT_EQ(circles[i].get_center().y, i * 10.0f);
        EXPECT_FLOAT_EQ(circles[i].get_radius(), 6.0f);
    }
    
    // Batch contains
    std::vector<Point2D> points;
    std::vector<uint8_t> results(count);  // Use uint8_t instead of bool
    
    for (size_t i = 0; i < count; ++i) {
        // Test with circle centers
        points.push_back(circles[i].get_center());
    }
    
    Circle::batch_contains(circles.data(), count, points.data(), reinterpret_cast<bool*>(results.data()));
    
    // All points should be contained
    for (size_t i = 0; i < count; ++i) {
        EXPECT_TRUE(results[i] != 0);
    }
}

// Test edge cases
TEST_F(CircleTest, EdgeCases) {
    // Zero radius circle
    Circle zero_circle(10.0f, 10.0f, 0.0f);
    EXPECT_EQ(zero_circle.area(), 0.0f);
    EXPECT_EQ(zero_circle.circumference(), 0.0f);
    EXPECT_TRUE(zero_circle.contains_point(Point2D(10.0f, 10.0f)));
    EXPECT_FALSE(zero_circle.contains_point(Point2D(10.1f, 10.0f)));
    
    // Negative radius (should be handled gracefully)
    Circle neg_circle(0.0f, 0.0f, -5.0f);
    neg_circle.set_radius(5.0f);  // Fix it
    EXPECT_EQ(neg_circle.get_radius(), 5.0f);
    EXPECT_EQ(neg_circle.get_radius_squared(), 25.0f);
    
    // Very large circle
    Circle large_circle(0.0f, 0.0f, 1e6f);
    EXPECT_FLOAT_EQ(large_circle.area(), M_PI * 1e12f);
    
    // Circle at origin
    Circle origin_circle(0.0f, 0.0f, 1.0f);
    EXPECT_TRUE(origin_circle.contains_point(Point2D(0.0f, 0.0f)));
    EXPECT_TRUE(origin_circle.contains_point(Point2D(0.5f, 0.5f)));
}

// Test factory functions
TEST_F(CircleTest, FactoryFunctions) {
    auto circle1 = make_circle(Point2D(10.0f, 20.0f), 5.0f);
    ASSERT_NE(circle1, nullptr);
    EXPECT_EQ(circle1->get_center().x, 10.0f);
    EXPECT_EQ(circle1->get_center().y, 20.0f);
    EXPECT_EQ(circle1->get_radius(), 5.0f);
    
    auto circle2 = make_circle(15.0f, 25.0f, 7.0f);
    ASSERT_NE(circle2, nullptr);
    EXPECT_EQ(circle2->get_center().x, 15.0f);
    EXPECT_EQ(circle2->get_center().y, 25.0f);
    EXPECT_EQ(circle2->get_radius(), 7.0f);
}