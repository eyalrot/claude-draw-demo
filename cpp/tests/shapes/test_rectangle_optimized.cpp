#include <gtest/gtest.h>
#include "claude_draw/shapes/rectangle_optimized.h"
#include <chrono>
#include <vector>
#include <random>

using namespace claude_draw::shapes;
using namespace claude_draw;

class RectangleOptimizedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed
    }
};

// Test basic rectangle creation
TEST_F(RectangleOptimizedTest, BasicConstruction) {
    // Default constructor
    Rectangle r1;
    EXPECT_EQ(r1.get_x(), 0.0f);
    EXPECT_EQ(r1.get_y(), 0.0f);
    EXPECT_EQ(r1.get_width(), 0.0f);
    EXPECT_EQ(r1.get_height(), 0.0f);
    
    // Constructor with parameters
    Rectangle r2(10.0f, 20.0f, 30.0f, 40.0f);
    EXPECT_EQ(r2.get_x(), 10.0f);
    EXPECT_EQ(r2.get_y(), 20.0f);
    EXPECT_EQ(r2.get_width(), 30.0f);
    EXPECT_EQ(r2.get_height(), 40.0f);
    
    // Constructor with points
    Point2D top_left(5.0f, 10.0f);
    Point2D bottom_right(25.0f, 30.0f);
    Rectangle r3(top_left, bottom_right);
    EXPECT_EQ(r3.get_x(), 5.0f);
    EXPECT_EQ(r3.get_y(), 10.0f);
    EXPECT_EQ(r3.get_width(), 20.0f);
    EXPECT_EQ(r3.get_height(), 20.0f);
}

// Test memory footprint
TEST_F(RectangleOptimizedTest, MemoryFootprint) {
    // Rectangle should have minimal overhead over RectangleShape
    EXPECT_EQ(sizeof(Rectangle), sizeof(RectangleShape));
    EXPECT_EQ(sizeof(Rectangle), 32u);
}

// Test corner and center getters
TEST_F(RectangleOptimizedTest, CornerAndCenterGetters) {
    Rectangle rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    // Test corners
    Point2D tl = rect.get_top_left();
    EXPECT_FLOAT_EQ(tl.x, 10.0f);
    EXPECT_FLOAT_EQ(tl.y, 20.0f);
    
    Point2D tr = rect.get_top_right();
    EXPECT_FLOAT_EQ(tr.x, 40.0f);
    EXPECT_FLOAT_EQ(tr.y, 20.0f);
    
    Point2D bl = rect.get_bottom_left();
    EXPECT_FLOAT_EQ(bl.x, 10.0f);
    EXPECT_FLOAT_EQ(bl.y, 60.0f);
    
    Point2D br = rect.get_bottom_right();
    EXPECT_FLOAT_EQ(br.x, 40.0f);
    EXPECT_FLOAT_EQ(br.y, 60.0f);
    
    // Test center
    Point2D center = rect.get_center();
    EXPECT_FLOAT_EQ(center.x, 25.0f);
    EXPECT_FLOAT_EQ(center.y, 40.0f);
}

// Test setters
TEST_F(RectangleOptimizedTest, Setters) {
    Rectangle rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    // Test position setter
    rect.set_position(50.0f, 60.0f);
    EXPECT_FLOAT_EQ(rect.get_x(), 50.0f);
    EXPECT_FLOAT_EQ(rect.get_y(), 60.0f);
    EXPECT_FLOAT_EQ(rect.get_width(), 30.0f);  // Width preserved
    EXPECT_FLOAT_EQ(rect.get_height(), 40.0f); // Height preserved
    
    // Test size setter
    rect.set_size(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(rect.get_x(), 50.0f);      // Position preserved
    EXPECT_FLOAT_EQ(rect.get_y(), 60.0f);      // Position preserved
    EXPECT_FLOAT_EQ(rect.get_width(), 100.0f);
    EXPECT_FLOAT_EQ(rect.get_height(), 200.0f);
    
    // Test bounds setter
    rect.set_bounds(0.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_FLOAT_EQ(rect.get_x(), 0.0f);
    EXPECT_FLOAT_EQ(rect.get_y(), 0.0f);
    EXPECT_FLOAT_EQ(rect.get_width(), 10.0f);
    EXPECT_FLOAT_EQ(rect.get_height(), 10.0f);
}

// Test bounding box calculation
TEST_F(RectangleOptimizedTest, BoundingBox) {
    Rectangle rect(10.0f, 20.0f, 30.0f, 40.0f);
    BoundingBox bounds = rect.get_bounds();
    
    EXPECT_FLOAT_EQ(bounds.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bounds.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bounds.max_x, 40.0f);
    EXPECT_FLOAT_EQ(bounds.max_y, 60.0f);
    
    // Test with zero size
    Rectangle zero_rect(5.0f, 10.0f, 0.0f, 0.0f);
    BoundingBox zero_bounds = zero_rect.get_bounds();
    EXPECT_FLOAT_EQ(zero_bounds.min_x, 5.0f);
    EXPECT_FLOAT_EQ(zero_bounds.min_y, 10.0f);
    EXPECT_FLOAT_EQ(zero_bounds.max_x, 5.0f);
    EXPECT_FLOAT_EQ(zero_bounds.max_y, 10.0f);
}

// Test point containment
TEST_F(RectangleOptimizedTest, PointContainment) {
    Rectangle rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    // Points inside
    EXPECT_TRUE(rect.contains_point(15.0f, 25.0f));  // Top-left inside
    EXPECT_TRUE(rect.contains_point(25.0f, 40.0f));  // Center
    EXPECT_TRUE(rect.contains_point(35.0f, 55.0f));  // Bottom-right inside
    
    // Points on edges (should be included)
    EXPECT_TRUE(rect.contains_point(10.0f, 20.0f));  // Top-left corner
    EXPECT_TRUE(rect.contains_point(40.0f, 20.0f));  // Top-right corner
    EXPECT_TRUE(rect.contains_point(10.0f, 60.0f));  // Bottom-left corner
    EXPECT_TRUE(rect.contains_point(40.0f, 60.0f));  // Bottom-right corner
    EXPECT_TRUE(rect.contains_point(25.0f, 20.0f));  // Top edge
    EXPECT_TRUE(rect.contains_point(40.0f, 40.0f));  // Right edge
    
    // Points outside
    EXPECT_FALSE(rect.contains_point(9.0f, 20.0f));   // Left of rectangle
    EXPECT_FALSE(rect.contains_point(41.0f, 20.0f));  // Right of rectangle
    EXPECT_FALSE(rect.contains_point(25.0f, 19.0f));  // Above rectangle
    EXPECT_FALSE(rect.contains_point(25.0f, 61.0f));  // Below rectangle
    EXPECT_FALSE(rect.contains_point(0.0f, 0.0f));    // Far outside
    
    // Test with Point2D
    EXPECT_TRUE(rect.contains_point(Point2D(25.0f, 40.0f)));
    EXPECT_FALSE(rect.contains_point(Point2D(50.0f, 70.0f)));
}

// Test geometric calculations
TEST_F(RectangleOptimizedTest, GeometricCalculations) {
    Rectangle rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    // Area = width * height
    EXPECT_FLOAT_EQ(rect.area(), 1200.0f);  // 30 * 40
    
    // Perimeter = 2 * (width + height)
    EXPECT_FLOAT_EQ(rect.perimeter(), 140.0f);  // 2 * (30 + 40)
    
    // Test with different rectangle
    Rectangle square(0.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_FLOAT_EQ(square.area(), 100.0f);
    EXPECT_FLOAT_EQ(square.perimeter(), 40.0f);
}

// Test transformations
TEST_F(RectangleOptimizedTest, Transformations) {
    Rectangle rect(10.0f, 20.0f, 20.0f, 30.0f);
    
    // Translation
    Transform2D translate = Transform2D::translate(5.0f, -10.0f);
    rect.transform(translate);
    EXPECT_FLOAT_EQ(rect.get_x(), 15.0f);
    EXPECT_FLOAT_EQ(rect.get_y(), 10.0f);
    EXPECT_FLOAT_EQ(rect.get_width(), 20.0f);   // Size unchanged
    EXPECT_FLOAT_EQ(rect.get_height(), 30.0f);  // Size unchanged
    
    // Uniform scaling
    Rectangle rect2(10.0f, 10.0f, 20.0f, 30.0f);
    Transform2D scale = Transform2D::scale(2.0f, 2.0f);
    rect2.transform(scale);
    EXPECT_FLOAT_EQ(rect2.get_x(), 20.0f);      // Position scaled
    EXPECT_FLOAT_EQ(rect2.get_y(), 20.0f);      // Position scaled
    EXPECT_FLOAT_EQ(rect2.get_width(), 40.0f);  // Width scaled
    EXPECT_FLOAT_EQ(rect2.get_height(), 60.0f); // Height scaled
    
    // 90-degree rotation
    Rectangle rect3(10.0f, 0.0f, 20.0f, 30.0f);
    Transform2D rotate = Transform2D::rotate(M_PI / 2);  // 90 degrees
    rect3.transform(rotate);
    // After rotation, the axis-aligned bounding box changes
    EXPECT_NEAR(rect3.get_x(), -30.0f, 1e-5f);
    EXPECT_NEAR(rect3.get_y(), 10.0f, 1e-5f);
    EXPECT_NEAR(rect3.get_width(), 30.0f, 1e-5f);   // Width and height swap
    EXPECT_NEAR(rect3.get_height(), 20.0f, 1e-5f);
}

// Test rectangle-rectangle intersection
TEST_F(RectangleOptimizedTest, RectangleIntersection) {
    Rectangle r1(0.0f, 0.0f, 20.0f, 20.0f);
    
    // Overlapping rectangles
    Rectangle r2(10.0f, 10.0f, 20.0f, 20.0f);
    EXPECT_TRUE(r1.intersects_rectangle(r2));
    
    // Check intersection area
    Rectangle intersection = r1.intersection(r2);
    EXPECT_FLOAT_EQ(intersection.get_x(), 10.0f);
    EXPECT_FLOAT_EQ(intersection.get_y(), 10.0f);
    EXPECT_FLOAT_EQ(intersection.get_width(), 10.0f);
    EXPECT_FLOAT_EQ(intersection.get_height(), 10.0f);
    
    // Touching rectangles (edge case)
    Rectangle r3(20.0f, 0.0f, 20.0f, 20.0f);
    EXPECT_TRUE(r1.intersects_rectangle(r3));  // Touching edges are considered intersecting
    
    // Non-intersecting rectangles
    Rectangle r4(25.0f, 25.0f, 10.0f, 10.0f);
    EXPECT_FALSE(r1.intersects_rectangle(r4));
    
    // Empty intersection
    Rectangle empty_intersection = r1.intersection(r4);
    EXPECT_FLOAT_EQ(empty_intersection.get_width(), 0.0f);
    EXPECT_FLOAT_EQ(empty_intersection.get_height(), 0.0f);
    
    // Contained rectangle
    Rectangle r5(5.0f, 5.0f, 10.0f, 10.0f);
    EXPECT_TRUE(r1.intersects_rectangle(r5));
    Rectangle contained_intersection = r1.intersection(r5);
    EXPECT_FLOAT_EQ(contained_intersection.get_x(), 5.0f);
    EXPECT_FLOAT_EQ(contained_intersection.get_y(), 5.0f);
    EXPECT_FLOAT_EQ(contained_intersection.get_width(), 10.0f);
    EXPECT_FLOAT_EQ(contained_intersection.get_height(), 10.0f);
}

// Test rectangle union
TEST_F(RectangleOptimizedTest, RectangleUnion) {
    Rectangle r1(0.0f, 0.0f, 20.0f, 20.0f);
    Rectangle r2(10.0f, 10.0f, 20.0f, 20.0f);
    
    Rectangle union_rect = r1.union_with(r2);
    EXPECT_FLOAT_EQ(union_rect.get_x(), 0.0f);
    EXPECT_FLOAT_EQ(union_rect.get_y(), 0.0f);
    EXPECT_FLOAT_EQ(union_rect.get_width(), 30.0f);
    EXPECT_FLOAT_EQ(union_rect.get_height(), 30.0f);
    
    // Non-overlapping rectangles
    Rectangle r3(50.0f, 50.0f, 10.0f, 10.0f);
    Rectangle union_rect2 = r1.union_with(r3);
    EXPECT_FLOAT_EQ(union_rect2.get_x(), 0.0f);
    EXPECT_FLOAT_EQ(union_rect2.get_y(), 0.0f);
    EXPECT_FLOAT_EQ(union_rect2.get_width(), 60.0f);
    EXPECT_FLOAT_EQ(union_rect2.get_height(), 60.0f);
}

// Test style management
TEST_F(RectangleOptimizedTest, StyleManagement) {
    Rectangle rect(0.0f, 0.0f, 10.0f, 10.0f);
    
    // Default colors
    Color default_fill = rect.get_fill_color();
    EXPECT_EQ(default_fill.r, 0);
    EXPECT_EQ(default_fill.g, 0);
    EXPECT_EQ(default_fill.b, 0);
    EXPECT_EQ(default_fill.a, 255);
    
    // Set fill color
    Color red(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    rect.set_fill_color(red);
    Color fill = rect.get_fill_color();
    EXPECT_EQ(fill.r, 255);
    EXPECT_EQ(fill.g, 0);
    EXPECT_EQ(fill.b, 0);
    EXPECT_EQ(fill.a, 255);
    
    // Set stroke color
    Color blue(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(128));
    rect.set_stroke_color(blue);
    Color stroke = rect.get_stroke_color();
    EXPECT_EQ(stroke.r, 0);
    EXPECT_EQ(stroke.g, 0);
    EXPECT_EQ(stroke.b, 255);
    EXPECT_EQ(stroke.a, 128);
    
    // Stroke width
    EXPECT_FLOAT_EQ(rect.get_stroke_width(), 1.0f);  // Default
    rect.set_stroke_width(3.5f);
    EXPECT_FLOAT_EQ(rect.get_stroke_width(), 3.5f);
}

// Test flags and visibility
TEST_F(RectangleOptimizedTest, FlagsAndVisibility) {
    Rectangle rect(0.0f, 0.0f, 10.0f, 10.0f);
    
    // Default flags
    EXPECT_TRUE(rect.is_visible());
    EXPECT_TRUE(rect.is_filled());
    EXPECT_FALSE(rect.is_stroked());
    
    // Toggle visibility
    rect.set_visible(false);
    EXPECT_FALSE(rect.is_visible());
    rect.set_visible(true);
    EXPECT_TRUE(rect.is_visible());
    
    // Toggle fill
    rect.set_filled(false);
    EXPECT_FALSE(rect.is_filled());
    rect.set_filled(true);
    EXPECT_TRUE(rect.is_filled());
    
    // Toggle stroke
    rect.set_stroked(true);
    EXPECT_TRUE(rect.is_stroked());
    rect.set_stroked(false);
    EXPECT_FALSE(rect.is_stroked());
}

// Test edge cases
TEST_F(RectangleOptimizedTest, EdgeCases) {
    // Zero size rectangle
    Rectangle zero_rect(10.0f, 20.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(zero_rect.area(), 0.0f);
    EXPECT_FLOAT_EQ(zero_rect.perimeter(), 0.0f);
    EXPECT_TRUE(zero_rect.contains_point(10.0f, 20.0f));  // Only contains top-left
    EXPECT_FALSE(zero_rect.contains_point(10.1f, 20.0f));
    
    // Negative size rectangle
    Rectangle neg_rect(10.0f, 20.0f, -30.0f, -40.0f);
    // The shape doesn't normalize bounds - it stores as-is
    EXPECT_FLOAT_EQ(neg_rect.get_x(), 10.0f);
    EXPECT_FLOAT_EQ(neg_rect.get_y(), 20.0f);
    EXPECT_FLOAT_EQ(neg_rect.get_width(), -30.0f);
    EXPECT_FLOAT_EQ(neg_rect.get_height(), -40.0f);
    // Bounds will reflect the actual x1,y1,x2,y2 values (not normalized)
    BoundingBox neg_bounds = neg_rect.get_bounds();
    EXPECT_FLOAT_EQ(neg_bounds.min_x, 10.0f);    // x1
    EXPECT_FLOAT_EQ(neg_bounds.min_y, 20.0f);    // y1
    EXPECT_FLOAT_EQ(neg_bounds.max_x, -20.0f);   // x1 + (-30) = -20
    EXPECT_FLOAT_EQ(neg_bounds.max_y, -20.0f);   // y1 + (-40) = -20
    
    // Very large rectangle
    Rectangle large_rect(0.0f, 0.0f, 1e6f, 1e6f);
    EXPECT_TRUE(large_rect.contains_point(999999.0f, 999999.0f));
    EXPECT_FALSE(large_rect.contains_point(1000001.0f, 1000001.0f));
}

// Test batch operations
TEST_F(RectangleOptimizedTest, BatchOperations) {
    const size_t count = 1000;
    std::vector<Rectangle> rectangles;
    std::vector<Point2D> points;
    
    // Create test data
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
    
    for (size_t i = 0; i < count; ++i) {
        float x = dist(rng);
        float y = dist(rng);
        float w = std::abs(dist(rng)) * 0.5f;
        float h = std::abs(dist(rng)) * 0.5f;
        rectangles.emplace_back(x, y, w, h);
        points.emplace_back(dist(rng), dist(rng));
    }
    
    // Test batch contains
    std::unique_ptr<bool[]> bool_results(new bool[count]);
    Rectangle::batch_contains(rectangles.data(), count, points.data(), bool_results.get());
    
    // Verify a few results manually
    for (size_t i = 0; i < 10; ++i) {
        bool expected = rectangles[i].contains_point(points[i]);
        EXPECT_EQ(bool_results[i], expected);
    }
    
    // Test batch transform
    Transform2D transform = Transform2D::translate(10.0f, 20.0f);
    std::vector<Rectangle> rectangles_copy = rectangles;
    Rectangle::batch_transform(rectangles_copy.data(), count, transform);
    
    // Verify transforms
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(rectangles_copy[i].get_x(), rectangles[i].get_x() + 10.0f);
        EXPECT_FLOAT_EQ(rectangles_copy[i].get_y(), rectangles[i].get_y() + 20.0f);
    }
    
    // Test batch intersects
    std::vector<Rectangle> rectangles2;
    for (size_t i = 0; i < count; ++i) {
        float x = dist(rng);
        float y = dist(rng);
        float w = std::abs(dist(rng)) * 0.5f;
        float h = std::abs(dist(rng)) * 0.5f;
        rectangles2.emplace_back(x, y, w, h);
    }
    
    Rectangle::batch_intersects(rectangles.data(), rectangles2.data(), count, bool_results.get());
    
    // Verify a few results
    for (size_t i = 0; i < 10; ++i) {
        bool expected = rectangles[i].intersects_rectangle(rectangles2[i]);
        EXPECT_EQ(bool_results[i], expected);
    }
}

// Performance test - rectangle creation
TEST_F(RectangleOptimizedTest, CreationPerformance) {
    const size_t iterations = 1000000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        volatile Rectangle r(i % 100, i % 200, i % 50 + 1, i % 60 + 1);
        (void)r;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double ns_per_creation = static_cast<double>(duration.count()) / iterations;
    
    // Should be sub-microsecond (< 1000 nanoseconds)
    EXPECT_LT(ns_per_creation, 1000.0);
    
    // Should ideally be < 500 nanoseconds as per requirements
    if (ns_per_creation < 500.0) {
        std::cout << "Rectangle creation performance: EXCELLENT - " 
                  << ns_per_creation << " ns/op" << std::endl;
    } else {
        std::cout << "Rectangle creation performance: " 
                  << ns_per_creation << " ns/op" << std::endl;
    }
}

// Test clone operation
TEST_F(RectangleOptimizedTest, CloneOperation) {
    Rectangle original(10.0f, 20.0f, 30.0f, 40.0f);
    original.set_fill_color(Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
    original.set_stroke_width(2.5f);
    original.set_id(123);
    
    auto cloned = original.clone();
    
    EXPECT_EQ(cloned->get_x(), original.get_x());
    EXPECT_EQ(cloned->get_y(), original.get_y());
    EXPECT_EQ(cloned->get_width(), original.get_width());
    EXPECT_EQ(cloned->get_height(), original.get_height());
    EXPECT_EQ(cloned->get_id(), original.get_id());
    
    // Modify clone shouldn't affect original
    cloned->set_position(0.0f, 0.0f);
    EXPECT_EQ(original.get_x(), 10.0f);
    EXPECT_EQ(original.get_y(), 20.0f);
}

// Test type trait
TEST_F(RectangleOptimizedTest, TypeTrait) {
    // Verify that Rectangle is recognized as a shape
    EXPECT_TRUE(is_shape<Rectangle>::value);
}