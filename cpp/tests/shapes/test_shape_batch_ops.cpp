#include <gtest/gtest.h>
#include "claude_draw/shapes/shape_batch_ops.h"
#include <chrono>
#include <random>
#include <memory>

using namespace claude_draw::shapes;
using namespace claude_draw::shapes::batch;
using namespace claude_draw;

class ShapeBatchOpsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test shapes
        circles_.push_back(Circle(10.0f, 10.0f, 5.0f));
        circles_.push_back(Circle(20.0f, 20.0f, 10.0f));
        circles_.push_back(Circle(30.0f, 30.0f, 15.0f));
        
        rectangles_.push_back(Rectangle(0.0f, 0.0f, 10.0f, 10.0f));
        rectangles_.push_back(Rectangle(10.0f, 10.0f, 20.0f, 20.0f));
        rectangles_.push_back(Rectangle(20.0f, 20.0f, 30.0f, 30.0f));
        
        ellipses_.push_back(Ellipse(15.0f, 15.0f, 10.0f, 5.0f));
        ellipses_.push_back(Ellipse(25.0f, 25.0f, 15.0f, 10.0f));
        
        lines_.push_back(Line(0.0f, 0.0f, 10.0f, 10.0f));
        lines_.push_back(Line(10.0f, 10.0f, 20.0f, 20.0f));
    }
    
    std::vector<Circle> circles_;
    std::vector<Rectangle> rectangles_;
    std::vector<Ellipse> ellipses_;
    std::vector<Line> lines_;
};

// Test mixed shape bounds calculation
TEST_F(ShapeBatchOpsTest, MixedShapeBoundsCalculation) {
    // Create array of shape pointers and types
    const void* shapes[] = {
        &circles_[0], &rectangles_[0], &ellipses_[0], &lines_[0],
        &circles_[1], &rectangles_[1]
    };
    
    ShapeType types[] = {
        ShapeType::Circle, ShapeType::Rectangle, ShapeType::Ellipse, ShapeType::Line,
        ShapeType::Circle, ShapeType::Rectangle
    };
    
    BoundingBox results[6];
    
    calculate_bounds(shapes, types, 6, results);
    
    // Verify circle bounds
    EXPECT_FLOAT_EQ(results[0].min_x, 5.0f);
    EXPECT_FLOAT_EQ(results[0].min_y, 5.0f);
    EXPECT_FLOAT_EQ(results[0].max_x, 15.0f);
    EXPECT_FLOAT_EQ(results[0].max_y, 15.0f);
    
    // Verify rectangle bounds
    EXPECT_FLOAT_EQ(results[1].min_x, 0.0f);
    EXPECT_FLOAT_EQ(results[1].min_y, 0.0f);
    EXPECT_FLOAT_EQ(results[1].max_x, 10.0f);
    EXPECT_FLOAT_EQ(results[1].max_y, 10.0f);
    
    // Verify ellipse bounds
    EXPECT_FLOAT_EQ(results[2].min_x, 5.0f);
    EXPECT_FLOAT_EQ(results[2].min_y, 10.0f);
    EXPECT_FLOAT_EQ(results[2].max_x, 25.0f);
    EXPECT_FLOAT_EQ(results[2].max_y, 20.0f);
    
    // Verify line bounds
    EXPECT_FLOAT_EQ(results[3].min_x, 0.0f);
    EXPECT_FLOAT_EQ(results[3].min_y, 0.0f);
    EXPECT_FLOAT_EQ(results[3].max_x, 10.0f);
    EXPECT_FLOAT_EQ(results[3].max_y, 10.0f);
}

// Test mixed shape transformation
TEST_F(ShapeBatchOpsTest, MixedShapeTransformation) {
    // Create copies for transformation
    Circle circle = circles_[0];
    Rectangle rect = rectangles_[0];
    Ellipse ellipse = ellipses_[0];
    Line line = lines_[0];
    
    void* shapes[] = { &circle, &rect, &ellipse, &line };
    ShapeType types[] = { ShapeType::Circle, ShapeType::Rectangle, 
                          ShapeType::Ellipse, ShapeType::Line };
    
    Transform2D transform = Transform2D::translate(10.0f, 20.0f);
    transform_shapes(shapes, types, 4, transform);
    
    // Verify transformations
    EXPECT_FLOAT_EQ(circle.get_center_x(), 20.0f);  // 10 + 10
    EXPECT_FLOAT_EQ(circle.get_center_y(), 30.0f);  // 10 + 20
    
    EXPECT_FLOAT_EQ(rect.get_x(), 10.0f);  // 0 + 10
    EXPECT_FLOAT_EQ(rect.get_y(), 20.0f);  // 0 + 20
    
    EXPECT_FLOAT_EQ(ellipse.get_center_x(), 25.0f);  // 15 + 10
    EXPECT_FLOAT_EQ(ellipse.get_center_y(), 35.0f);  // 15 + 20
    
    EXPECT_FLOAT_EQ(line.get_x1(), 10.0f);  // 0 + 10
    EXPECT_FLOAT_EQ(line.get_y1(), 20.0f);  // 0 + 20
}

// Test point containment for mixed shapes
TEST_F(ShapeBatchOpsTest, MixedShapePointContainment) {
    const void* shapes[] = {
        &circles_[0], &rectangles_[0], &ellipses_[0], &lines_[0]
    };
    
    ShapeType types[] = {
        ShapeType::Circle, ShapeType::Rectangle, ShapeType::Ellipse, ShapeType::Line
    };
    
    Point2D points[] = {
        Point2D(10.0f, 10.0f),  // Inside circle
        Point2D(5.0f, 5.0f),    // Inside rectangle
        Point2D(15.0f, 15.0f),  // Inside ellipse
        Point2D(5.0f, 5.0f)     // On line
    };
    
    bool results[4];
    contains_points(shapes, types, 4, points, results);
    
    EXPECT_TRUE(results[0]);   // Circle contains center
    EXPECT_TRUE(results[1]);   // Rectangle contains (5,5)
    EXPECT_TRUE(results[2]);   // Ellipse contains center
    EXPECT_TRUE(results[3]);   // Line contains midpoint
}

// Test union bounds calculation
TEST_F(ShapeBatchOpsTest, UnionBoundsCalculation) {
    const void* shapes[] = {
        &circles_[0], &rectangles_[1], &ellipses_[0]
    };
    
    ShapeType types[] = {
        ShapeType::Circle, ShapeType::Rectangle, ShapeType::Ellipse
    };
    
    BoundingBox union_bounds = calculate_union_bounds(shapes, types, 3);
    
    // Circle: (5,5) to (15,15)
    // Rectangle: (10,10) to (30,30)
    // Ellipse: (5,10) to (25,20)
    // Union should be (5,5) to (30,30)
    
    EXPECT_FLOAT_EQ(union_bounds.min_x, 5.0f);
    EXPECT_FLOAT_EQ(union_bounds.min_y, 5.0f);
    EXPECT_FLOAT_EQ(union_bounds.max_x, 30.0f);
    EXPECT_FLOAT_EQ(union_bounds.max_y, 30.0f);
}

// Test visibility filtering
TEST_F(ShapeBatchOpsTest, VisibilityFiltering) {
    // Set some shapes invisible
    circles_[1].set_visible(false);
    rectangles_[0].set_visible(false);
    
    const void* shapes[] = {
        &circles_[0], &circles_[1], &rectangles_[0], &rectangles_[1]
    };
    
    ShapeType types[] = {
        ShapeType::Circle, ShapeType::Circle, 
        ShapeType::Rectangle, ShapeType::Rectangle
    };
    
    void* visible_shapes[4];
    ShapeType visible_types[4];
    
    size_t visible_count = filter_visible(shapes, types, 4, visible_shapes, visible_types);
    
    EXPECT_EQ(visible_count, 2u);
    EXPECT_EQ(visible_shapes[0], &circles_[0]);
    EXPECT_EQ(visible_shapes[1], &rectangles_[1]);
    EXPECT_EQ(visible_types[0], ShapeType::Circle);
    EXPECT_EQ(visible_types[1], ShapeType::Rectangle);
}

// Test batch color operations
TEST_F(ShapeBatchOpsTest, BatchColorOperations) {
    void* shapes[] = {
        &circles_[0], &rectangles_[0], &ellipses_[0], &lines_[0]
    };
    
    ShapeType types[] = {
        ShapeType::Circle, ShapeType::Rectangle, 
        ShapeType::Ellipse, ShapeType::Line
    };
    
    Color red(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    Color blue(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(255));
    
    // Set fill colors (lines don't have fill)
    set_fill_colors(shapes, types, 4, red);
    
    EXPECT_EQ(circles_[0].get_fill_color().r, 255);
    EXPECT_EQ(rectangles_[0].get_fill_color().r, 255);
    EXPECT_EQ(ellipses_[0].get_fill_color().r, 255);
    
    // Set stroke colors (all shapes have stroke)
    set_stroke_colors(shapes, types, 4, blue);
    
    EXPECT_EQ(circles_[0].get_stroke_color().b, 255);
    EXPECT_EQ(rectangles_[0].get_stroke_color().b, 255);
    EXPECT_EQ(ellipses_[0].get_stroke_color().b, 255);
    EXPECT_EQ(lines_[0].get_stroke_color().b, 255);
    
    // Set stroke widths
    set_stroke_widths(shapes, types, 4, 3.0f);
    
    EXPECT_FLOAT_EQ(circles_[0].get_stroke_width(), 3.0f);
    EXPECT_FLOAT_EQ(rectangles_[0].get_stroke_width(), 3.0f);
    EXPECT_FLOAT_EQ(ellipses_[0].get_stroke_width(), 3.0f);
    EXPECT_FLOAT_EQ(lines_[0].get_stroke_width(), 3.0f);
}

// Test typed batch operations
TEST_F(ShapeBatchOpsTest, TypedBatchOperations) {
    // Test circle batch operations
    const size_t count = 100;
    std::vector<Circle> circles;
    
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(i * 10.0f, i * 10.0f, 5.0f);
    }
    
    std::vector<BoundingBox> bounds(count);
    TypedBatchOps<Circle>::calculate_bounds(circles.data(), count, bounds.data());
    
    // Verify a few bounds
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(bounds[i].min_x, i * 10.0f - 5.0f);
        EXPECT_FLOAT_EQ(bounds[i].max_x, i * 10.0f + 5.0f);
    }
    
    // Test batch transformation
    Transform2D scale = Transform2D::scale(2.0f, 2.0f);
    TypedBatchOps<Circle>::transform(circles.data(), count, scale);
    
    // Verify transformations
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(circles[i].get_center_x(), i * 20.0f);
        EXPECT_FLOAT_EQ(circles[i].get_radius(), 10.0f);
    }
    
    // Test batch color setting
    Color green(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    TypedBatchOps<Circle>::set_fill_colors(circles.data(), count, green);
    
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(circles[i].get_fill_color().g, 255);
    }
}

// Test ShapeCollection
TEST_F(ShapeBatchOpsTest, ShapeCollection) {
    ShapeCollection collection;
    
    // Add various shapes
    collection.add_circle(std::make_unique<Circle>(10.0f, 10.0f, 5.0f));
    collection.add_rectangle(std::make_unique<Rectangle>(0.0f, 0.0f, 20.0f, 20.0f));
    collection.add_ellipse(std::make_unique<Ellipse>(15.0f, 15.0f, 10.0f, 5.0f));
    collection.add_line(std::make_unique<Line>(0.0f, 0.0f, 30.0f, 30.0f));
    
    EXPECT_EQ(collection.size(), 4u);
    EXPECT_FALSE(collection.empty());
    
    // Test transform all
    Transform2D translate = Transform2D::translate(5.0f, 5.0f);
    collection.transform_all(translate);
    
    // Test calculate all bounds
    std::vector<BoundingBox> all_bounds = collection.calculate_all_bounds();
    EXPECT_EQ(all_bounds.size(), 4u);
    
    // Circle was at (10,10) with radius 5, translated to (15,15)
    EXPECT_FLOAT_EQ(all_bounds[0].min_x, 10.0f);  // 15 - 5
    EXPECT_FLOAT_EQ(all_bounds[0].max_x, 20.0f);  // 15 + 5
    
    // Test union bounds
    BoundingBox union_bounds = collection.calculate_union_bounds();
    EXPECT_FLOAT_EQ(union_bounds.min_x, 5.0f);   // Rectangle min after translation
    EXPECT_FLOAT_EQ(union_bounds.max_x, 35.0f);  // Line max after translation
    
    // Test point containment
    std::vector<Point2D> test_points = {
        Point2D(15.0f, 15.0f),  // Circle center after translation
        Point2D(10.0f, 10.0f),  // Inside rectangle after translation  
        Point2D(20.0f, 20.0f),  // Ellipse center after translation
        Point2D(20.0f, 20.0f)   // On line after translation
    };
    
    std::vector<bool> contains_results = collection.contains_points(test_points);
    EXPECT_TRUE(contains_results[0]);   // Circle
    EXPECT_TRUE(contains_results[1]);   // Rectangle
    EXPECT_TRUE(contains_results[2]);   // Ellipse
    EXPECT_TRUE(contains_results[3]);   // Line (with tolerance)
    
    // Test clear
    collection.clear();
    EXPECT_EQ(collection.size(), 0u);
    EXPECT_TRUE(collection.empty());
}

// Test performance utilities
TEST_F(ShapeBatchOpsTest, PerformanceUtilities) {
    const size_t count = 1000;
    std::vector<Circle> circles;
    
    // Create test data
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(i * 1.0f, i * 2.0f, 5.0f);
    }
    
    // Test prefetching (just ensure it compiles and runs)
    perf::prefetch_shapes(circles.data(), count);
    
    // Test chunk processing
    size_t process_count = 0;
    perf::process_in_chunks(circles.data(), count, 64,
        [&process_count](Circle& circle, size_t index) {
            circle.set_radius(10.0f);
            process_count++;
        });
    
    EXPECT_EQ(process_count, count);
    
    // Verify all circles were processed
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(circles[i].get_radius(), 10.0f);
    }
}

// Test SIMD bounds calculation
TEST_F(ShapeBatchOpsTest, SimdBoundsCalculation) {
    const size_t count = 100;
    std::vector<CircleShape> circle_shapes;
    std::vector<BoundingBox> results(count);
    
    // Create aligned data
    for (size_t i = 0; i < count; ++i) {
        circle_shapes.push_back(CircleShape(i * 10.0f, i * 5.0f, 3.0f));
    }
    
    // Test SIMD circle bounds
    simd::calculate_circle_bounds_simd(circle_shapes.data(), count, results.data());
    
    // Verify results
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(results[i].min_x, i * 10.0f - 3.0f);
        EXPECT_FLOAT_EQ(results[i].max_x, i * 10.0f + 3.0f);
        EXPECT_FLOAT_EQ(results[i].min_y, i * 5.0f - 3.0f);
        EXPECT_FLOAT_EQ(results[i].max_y, i * 5.0f + 3.0f);
    }
    
    // Test other shape types
    std::vector<RectangleShape> rect_shapes;
    for (size_t i = 0; i < count; ++i) {
        rect_shapes.push_back(RectangleShape(i * 1.0f, i * 2.0f, 10.0f, 20.0f));
    }
    
    simd::calculate_rectangle_bounds_simd(rect_shapes.data(), count, results.data());
    
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(results[i].min_x, i * 1.0f);
        EXPECT_FLOAT_EQ(results[i].max_x, i * 1.0f + 10.0f);
    }
}

// Performance test for batch operations
TEST_F(ShapeBatchOpsTest, BatchOperationPerformance) {
    const size_t count = 10000;
    std::vector<Circle> circles;
    std::vector<Rectangle> rectangles;
    
    // Create large dataset
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);
    
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(dist(rng), dist(rng), dist(rng) * 0.1f);
        rectangles.emplace_back(dist(rng), dist(rng), dist(rng) * 0.2f, dist(rng) * 0.2f);
    }
    
    // Create mixed shape arrays
    std::vector<const void*> shapes;
    std::vector<ShapeType> types;
    
    for (size_t i = 0; i < count; ++i) {
        shapes.push_back(&circles[i]);
        types.push_back(ShapeType::Circle);
        shapes.push_back(&rectangles[i]);
        types.push_back(ShapeType::Rectangle);
    }
    
    std::vector<BoundingBox> results(shapes.size());
    
    // Measure batch bounds calculation
    auto start = std::chrono::high_resolution_clock::now();
    
    calculate_bounds(shapes.data(), types.data(), shapes.size(), results.data());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double us_per_shape = static_cast<double>(duration.count()) / shapes.size();
    
    // Should be well under 1 microsecond per shape
    EXPECT_LT(us_per_shape, 1.0);
    
    std::cout << "Batch bounds calculation: " << us_per_shape 
              << " μs/shape for " << shapes.size() << " shapes" << std::endl;
}

// Test error handling
TEST_F(ShapeBatchOpsTest, ErrorHandling) {
    ShapeCollection collection;
    
    // Test mismatched points vector size
    collection.add_circle(std::make_unique<Circle>(10.0f, 10.0f, 5.0f));
    collection.add_rectangle(std::make_unique<Rectangle>(0.0f, 0.0f, 10.0f, 10.0f));
    
    std::vector<Point2D> wrong_size_points = { Point2D(5.0f, 5.0f) };  // Only 1 point for 2 shapes
    
    EXPECT_THROW(collection.contains_points(wrong_size_points), std::invalid_argument);
    
    // Test empty collection
    ShapeCollection empty_collection;
    BoundingBox empty_bounds = empty_collection.calculate_union_bounds();
    EXPECT_FLOAT_EQ(empty_bounds.min_x, 0.0f);
    EXPECT_FLOAT_EQ(empty_bounds.min_y, 0.0f);
    EXPECT_FLOAT_EQ(empty_bounds.max_x, 0.0f);
    EXPECT_FLOAT_EQ(empty_bounds.max_y, 0.0f);
}