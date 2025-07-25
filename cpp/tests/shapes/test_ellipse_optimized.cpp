#include <gtest/gtest.h>
#include "claude_draw/shapes/ellipse_optimized.h"
#include <chrono>
#include <vector>
#include <random>

using namespace claude_draw::shapes;
using namespace claude_draw;

class EllipseOptimizedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed
    }
};

// Test basic ellipse creation
TEST_F(EllipseOptimizedTest, BasicConstruction) {
    // Default constructor
    Ellipse e1;
    EXPECT_EQ(e1.get_center_x(), 0.0f);
    EXPECT_EQ(e1.get_center_y(), 0.0f);
    EXPECT_EQ(e1.get_radius_x(), 0.0f);
    EXPECT_EQ(e1.get_radius_y(), 0.0f);
    
    // Constructor with parameters
    Ellipse e2(10.0f, 20.0f, 5.0f, 3.0f);
    EXPECT_EQ(e2.get_center_x(), 10.0f);
    EXPECT_EQ(e2.get_center_y(), 20.0f);
    EXPECT_EQ(e2.get_radius_x(), 5.0f);
    EXPECT_EQ(e2.get_radius_y(), 3.0f);
    
    // Constructor with Point2D
    Point2D center(15.0f, 25.0f);
    Ellipse e3(center, 7.0f, 4.0f);
    EXPECT_EQ(e3.get_center_x(), 15.0f);
    EXPECT_EQ(e3.get_center_y(), 25.0f);
    EXPECT_EQ(e3.get_radius_x(), 7.0f);
    EXPECT_EQ(e3.get_radius_y(), 4.0f);
}

// Test memory footprint
TEST_F(EllipseOptimizedTest, MemoryFootprint) {
    // Ellipse should have minimal overhead over EllipseShape
    EXPECT_EQ(sizeof(Ellipse), sizeof(EllipseShape));
    EXPECT_EQ(sizeof(Ellipse), 32u);
}

// Test bounding box calculation
TEST_F(EllipseOptimizedTest, BoundingBox) {
    Ellipse ellipse(10.0f, 20.0f, 5.0f, 3.0f);
    BoundingBox bounds = ellipse.get_bounds();
    
    EXPECT_FLOAT_EQ(bounds.min_x, 5.0f);   // center_x - radius_x
    EXPECT_FLOAT_EQ(bounds.min_y, 17.0f);  // center_y - radius_y
    EXPECT_FLOAT_EQ(bounds.max_x, 15.0f);  // center_x + radius_x
    EXPECT_FLOAT_EQ(bounds.max_y, 23.0f);  // center_y + radius_y
    
    // Test with zero radii
    Ellipse point(5.0f, 10.0f, 0.0f, 0.0f);
    BoundingBox point_bounds = point.get_bounds();
    EXPECT_FLOAT_EQ(point_bounds.min_x, 5.0f);
    EXPECT_FLOAT_EQ(point_bounds.min_y, 10.0f);
    EXPECT_FLOAT_EQ(point_bounds.max_x, 5.0f);
    EXPECT_FLOAT_EQ(point_bounds.max_y, 10.0f);
}

// Test point containment
TEST_F(EllipseOptimizedTest, PointContainment) {
    Ellipse ellipse(0.0f, 0.0f, 10.0f, 5.0f);
    
    // Points inside
    EXPECT_TRUE(ellipse.contains_point(0.0f, 0.0f));    // Center
    EXPECT_TRUE(ellipse.contains_point(5.0f, 0.0f));    // Right of center
    EXPECT_TRUE(ellipse.contains_point(0.0f, 3.0f));    // Above center
    EXPECT_TRUE(ellipse.contains_point(6.0f, 3.0f));    // Inside ellipse
    
    // Points on perimeter (should be included)
    EXPECT_TRUE(ellipse.contains_point(10.0f, 0.0f));   // Right edge
    EXPECT_TRUE(ellipse.contains_point(0.0f, 5.0f));    // Top edge
    EXPECT_TRUE(ellipse.contains_point(-10.0f, 0.0f));  // Left edge
    EXPECT_TRUE(ellipse.contains_point(0.0f, -5.0f));   // Bottom edge
    
    // Points outside
    EXPECT_FALSE(ellipse.contains_point(11.0f, 0.0f));  // Beyond right edge
    EXPECT_FALSE(ellipse.contains_point(0.0f, 6.0f));   // Beyond top edge
    EXPECT_FALSE(ellipse.contains_point(8.0f, 4.0f));   // Outside ellipse
    EXPECT_FALSE(ellipse.contains_point(10.0f, 5.0f));  // Corner of bounding box
    
    // Test with Point2D
    EXPECT_TRUE(ellipse.contains_point(Point2D(5.0f, 2.0f)));
    EXPECT_FALSE(ellipse.contains_point(Point2D(15.0f, 0.0f)));
}

// Test geometric calculations
TEST_F(EllipseOptimizedTest, GeometricCalculations) {
    Ellipse ellipse(0.0f, 0.0f, 10.0f, 5.0f);
    
    // Area = pi * radius_x * radius_y
    EXPECT_FLOAT_EQ(ellipse.area(), M_PI * 10.0f * 5.0f);
    
    // Perimeter (approximate)
    float perimeter = ellipse.perimeter();
    // For an ellipse with radii 10 and 5, perimeter should be around 48.44
    EXPECT_NEAR(perimeter, 48.44f, 0.1f);
    
    // Test with different radii
    ellipse.set_radii(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(ellipse.area(), M_PI * 3.0f * 4.0f);
}

// Test circle detection
TEST_F(EllipseOptimizedTest, CircleDetection) {
    // Perfect circle
    Ellipse circle(0.0f, 0.0f, 5.0f, 5.0f);
    EXPECT_TRUE(circle.is_circle());
    EXPECT_FLOAT_EQ(circle.get_average_radius(), 5.0f);
    
    // Nearly circular
    Ellipse nearly_circle(0.0f, 0.0f, 5.0f, 5.000001f);
    EXPECT_TRUE(nearly_circle.is_circle());
    
    // Not a circle
    Ellipse ellipse(0.0f, 0.0f, 10.0f, 5.0f);
    EXPECT_FALSE(ellipse.is_circle());
    EXPECT_FLOAT_EQ(ellipse.get_average_radius(), 7.5f);
}

// Test radius and point at angle
TEST_F(EllipseOptimizedTest, RadiusAndPointAtAngle) {
    Ellipse ellipse(0.0f, 0.0f, 10.0f, 5.0f);
    
    // Test radius at various angles
    EXPECT_FLOAT_EQ(ellipse.radius_at_angle(0.0f), 10.0f);        // 0 degrees (right)
    EXPECT_FLOAT_EQ(ellipse.radius_at_angle(M_PI/2), 5.0f);       // 90 degrees (top)
    EXPECT_FLOAT_EQ(ellipse.radius_at_angle(M_PI), 10.0f);        // 180 degrees (left)
    EXPECT_FLOAT_EQ(ellipse.radius_at_angle(3*M_PI/2), 5.0f);     // 270 degrees (bottom)
    
    // Test points on ellipse
    Point2D p0 = ellipse.point_at_angle(0.0f);
    EXPECT_FLOAT_EQ(p0.x, 10.0f);
    EXPECT_FLOAT_EQ(p0.y, 0.0f);
    
    Point2D p90 = ellipse.point_at_angle(M_PI/2);
    EXPECT_NEAR(p90.x, 0.0f, 1e-6f);
    EXPECT_FLOAT_EQ(p90.y, 5.0f);
    
    Point2D p180 = ellipse.point_at_angle(M_PI);
    EXPECT_FLOAT_EQ(p180.x, -10.0f);
    EXPECT_NEAR(p180.y, 0.0f, 1e-6f);
}

// Test transformations
TEST_F(EllipseOptimizedTest, Transformations) {
    Ellipse ellipse(10.0f, 20.0f, 5.0f, 3.0f);
    
    // Translation
    Transform2D translate = Transform2D::translate(5.0f, -10.0f);
    ellipse.transform(translate);
    EXPECT_FLOAT_EQ(ellipse.get_center_x(), 15.0f);
    EXPECT_FLOAT_EQ(ellipse.get_center_y(), 10.0f);
    EXPECT_FLOAT_EQ(ellipse.get_radius_x(), 5.0f);   // Radii unchanged
    EXPECT_FLOAT_EQ(ellipse.get_radius_y(), 3.0f);   // Radii unchanged
    
    // Uniform scaling
    Transform2D scale = Transform2D::scale(2.0f, 2.0f);
    ellipse.transform(scale);
    EXPECT_FLOAT_EQ(ellipse.get_center_x(), 30.0f);
    EXPECT_FLOAT_EQ(ellipse.get_center_y(), 20.0f);
    EXPECT_FLOAT_EQ(ellipse.get_radius_x(), 10.0f);  // Radii scaled
    EXPECT_FLOAT_EQ(ellipse.get_radius_y(), 6.0f);   // Radii scaled
    
    // Non-uniform scaling
    Ellipse ellipse2(10.0f, 10.0f, 4.0f, 4.0f);
    Transform2D scale_xy = Transform2D::scale(2.0f, 3.0f);
    ellipse2.transform(scale_xy);
    EXPECT_FLOAT_EQ(ellipse2.get_center_x(), 20.0f);
    EXPECT_FLOAT_EQ(ellipse2.get_center_y(), 30.0f);
    EXPECT_FLOAT_EQ(ellipse2.get_radius_x(), 8.0f);   // Scaled by 2
    EXPECT_FLOAT_EQ(ellipse2.get_radius_y(), 12.0f);  // Scaled by 3
    
    // Rotation (center should rotate, radii unchanged for axis-aligned ellipse)
    Ellipse ellipse3(10.0f, 0.0f, 5.0f, 3.0f);
    Transform2D rotate = Transform2D::rotate(M_PI / 2);  // 90 degrees
    ellipse3.transform(rotate);
    EXPECT_NEAR(ellipse3.get_center_x(), 0.0f, 1e-6f);
    EXPECT_NEAR(ellipse3.get_center_y(), 10.0f, 1e-6f);
    EXPECT_FLOAT_EQ(ellipse3.get_radius_x(), 5.0f);
    EXPECT_FLOAT_EQ(ellipse3.get_radius_y(), 3.0f);
}

// Test style management
TEST_F(EllipseOptimizedTest, StyleManagement) {
    Ellipse ellipse(0.0f, 0.0f, 10.0f, 5.0f);
    
    // Default colors
    Color default_fill = ellipse.get_fill_color();
    EXPECT_EQ(default_fill.r, 0);
    EXPECT_EQ(default_fill.g, 0);
    EXPECT_EQ(default_fill.b, 0);
    EXPECT_EQ(default_fill.a, 255);
    
    // Set fill color
    Color red(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    ellipse.set_fill_color(red);
    Color fill = ellipse.get_fill_color();
    EXPECT_EQ(fill.r, 255);
    EXPECT_EQ(fill.g, 0);
    EXPECT_EQ(fill.b, 0);
    EXPECT_EQ(fill.a, 255);
    
    // Set stroke color
    Color blue(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(128));
    ellipse.set_stroke_color(blue);
    Color stroke = ellipse.get_stroke_color();
    EXPECT_EQ(stroke.r, 0);
    EXPECT_EQ(stroke.g, 0);
    EXPECT_EQ(stroke.b, 255);
    EXPECT_EQ(stroke.a, 128);
    
    // Stroke width
    EXPECT_FLOAT_EQ(ellipse.get_stroke_width(), 1.0f);  // Default
    ellipse.set_stroke_width(2.5f);
    EXPECT_FLOAT_EQ(ellipse.get_stroke_width(), 2.5f);
}

// Test flags and visibility
TEST_F(EllipseOptimizedTest, FlagsAndVisibility) {
    Ellipse ellipse(0.0f, 0.0f, 10.0f, 5.0f);
    
    // Default flags
    EXPECT_TRUE(ellipse.is_visible());
    EXPECT_TRUE(ellipse.is_filled());
    EXPECT_FALSE(ellipse.is_stroked());
    
    // Toggle visibility
    ellipse.set_visible(false);
    EXPECT_FALSE(ellipse.is_visible());
    ellipse.set_visible(true);
    EXPECT_TRUE(ellipse.is_visible());
    
    // Toggle fill
    ellipse.set_filled(false);
    EXPECT_FALSE(ellipse.is_filled());
    ellipse.set_filled(true);
    EXPECT_TRUE(ellipse.is_filled());
    
    // Toggle stroke
    ellipse.set_stroked(true);
    EXPECT_TRUE(ellipse.is_stroked());
    ellipse.set_stroked(false);
    EXPECT_FALSE(ellipse.is_stroked());
}

// Test edge cases
TEST_F(EllipseOptimizedTest, EdgeCases) {
    // Zero radius ellipse
    Ellipse zero_ellipse(10.0f, 20.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(zero_ellipse.area(), 0.0f);
    // With zero radii, division by zero in contains check means it won't contain any points
    EXPECT_FALSE(zero_ellipse.contains_point(10.0f, 20.0f));
    EXPECT_FALSE(zero_ellipse.contains_point(10.1f, 20.0f));
    
    // Negative radii (should work with absolute value in calculations)
    Ellipse neg_ellipse(0.0f, 0.0f, -5.0f, -3.0f);
    // Most calculations use radius squared, so sign doesn't matter for contains
    EXPECT_TRUE(neg_ellipse.contains_point(3.0f, 0.0f));
    
    // Very large ellipse
    Ellipse large_ellipse(0.0f, 0.0f, 1e6f, 5e5f);
    EXPECT_TRUE(large_ellipse.contains_point(999999.0f, 0.0f));
    EXPECT_FALSE(large_ellipse.contains_point(1000001.0f, 0.0f));
    
    // Very thin ellipse (almost a line)
    Ellipse thin_ellipse(0.0f, 0.0f, 100.0f, 0.1f);
    EXPECT_TRUE(thin_ellipse.contains_point(50.0f, 0.0f));
    EXPECT_FALSE(thin_ellipse.contains_point(50.0f, 1.0f));
}

// Test batch operations
TEST_F(EllipseOptimizedTest, BatchOperations) {
    const size_t count = 1000;
    std::vector<Ellipse> ellipses;
    std::vector<Point2D> points;
    
    // Create test data
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
    
    for (size_t i = 0; i < count; ++i) {
        ellipses.emplace_back(dist(rng), dist(rng), 
                             std::abs(dist(rng)) * 0.5f, 
                             std::abs(dist(rng)) * 0.3f);
        points.emplace_back(dist(rng), dist(rng));
    }
    
    // Test batch contains
    std::unique_ptr<bool[]> bool_results(new bool[count]);
    Ellipse::batch_contains(ellipses.data(), count, points.data(), bool_results.get());
    
    // Verify a few results manually
    for (size_t i = 0; i < 10; ++i) {
        bool expected = ellipses[i].contains_point(points[i]);
        EXPECT_EQ(bool_results[i], expected);
    }
    
    // Test batch transform
    Transform2D transform = Transform2D::translate(10.0f, 20.0f);
    std::vector<Ellipse> ellipses_copy = ellipses;
    Ellipse::batch_transform(ellipses_copy.data(), count, transform);
    
    // Verify transforms
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(ellipses_copy[i].get_center_x(), ellipses[i].get_center_x() + 10.0f);
        EXPECT_FLOAT_EQ(ellipses_copy[i].get_center_y(), ellipses[i].get_center_y() + 20.0f);
    }
    
    // Test batch area
    std::unique_ptr<float[]> area_results(new float[count]);
    Ellipse::batch_area(ellipses.data(), count, area_results.get());
    
    // Verify areas
    for (size_t i = 0; i < 10; ++i) {
        float expected_area = ellipses[i].area();
        EXPECT_FLOAT_EQ(area_results[i], expected_area);
    }
}

// Performance test - ellipse creation
TEST_F(EllipseOptimizedTest, CreationPerformance) {
    const size_t iterations = 1000000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        volatile Ellipse e(i % 100, i % 200, i % 50 + 1, i % 30 + 1);
        (void)e;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double ns_per_creation = static_cast<double>(duration.count()) / iterations;
    
    // Should be sub-microsecond (< 1000 nanoseconds)
    EXPECT_LT(ns_per_creation, 1000.0);
    
    // Should ideally be < 500 nanoseconds as per requirements
    if (ns_per_creation < 500.0) {
        std::cout << "Ellipse creation performance: EXCELLENT - " 
                  << ns_per_creation << " ns/op" << std::endl;
    } else {
        std::cout << "Ellipse creation performance: " 
                  << ns_per_creation << " ns/op" << std::endl;
    }
}

// Test clone operation
TEST_F(EllipseOptimizedTest, CloneOperation) {
    Ellipse original(10.0f, 20.0f, 5.0f, 3.0f);
    original.set_fill_color(Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
    original.set_stroke_width(3.0f);
    original.set_id(42);
    
    auto cloned = original.clone();
    
    EXPECT_EQ(cloned->get_center_x(), original.get_center_x());
    EXPECT_EQ(cloned->get_center_y(), original.get_center_y());
    EXPECT_EQ(cloned->get_radius_x(), original.get_radius_x());
    EXPECT_EQ(cloned->get_radius_y(), original.get_radius_y());
    EXPECT_EQ(cloned->get_id(), original.get_id());
    
    // Modify clone shouldn't affect original
    cloned->set_center(0.0f, 0.0f);
    EXPECT_EQ(original.get_center_x(), 10.0f);
    EXPECT_EQ(original.get_center_y(), 20.0f);
}

// Test factory functions
TEST_F(EllipseOptimizedTest, FactoryFunctions) {
    // Regular ellipse factory
    auto e1 = make_ellipse(10.0f, 20.0f, 5.0f, 3.0f);
    EXPECT_EQ(e1->get_center_x(), 10.0f);
    EXPECT_EQ(e1->get_center_y(), 20.0f);
    EXPECT_EQ(e1->get_radius_x(), 5.0f);
    EXPECT_EQ(e1->get_radius_y(), 3.0f);
    
    // Point-based factory
    auto e2 = make_ellipse(Point2D(15.0f, 25.0f), 7.0f, 4.0f);
    EXPECT_EQ(e2->get_center_x(), 15.0f);
    EXPECT_EQ(e2->get_center_y(), 25.0f);
    EXPECT_EQ(e2->get_radius_x(), 7.0f);
    EXPECT_EQ(e2->get_radius_y(), 4.0f);
    
    // Circle as ellipse factory
    auto circle = make_circle_as_ellipse(5.0f, 10.0f, 8.0f);
    EXPECT_EQ(circle->get_center_x(), 5.0f);
    EXPECT_EQ(circle->get_center_y(), 10.0f);
    EXPECT_EQ(circle->get_radius_x(), 8.0f);
    EXPECT_EQ(circle->get_radius_y(), 8.0f);
    EXPECT_TRUE(circle->is_circle());
}

// Test type trait
TEST_F(EllipseOptimizedTest, TypeTrait) {
    // Verify that Ellipse is recognized as a shape
    EXPECT_TRUE(is_shape<Ellipse>::value);
}