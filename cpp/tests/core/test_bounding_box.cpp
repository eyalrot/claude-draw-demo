#include <gtest/gtest.h>
#include "claude_draw/core/bounding_box.h"
#include "claude_draw/core/simd.h"
#include "../test_utils.h"
#include <vector>
#include <cmath>
#include <limits>

using namespace claude_draw;

class BoundingBoxTest : public ::testing::Test {
protected:
    static constexpr float EPSILON = 1e-5f;
    
    bool nearly_equal(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
    
    bool boxes_equal(const BoundingBox& a, const BoundingBox& b, float epsilon = EPSILON) {
        return nearly_equal(a.min_x, b.min_x, epsilon) &&
               nearly_equal(a.min_y, b.min_y, epsilon) &&
               nearly_equal(a.max_x, b.max_x, epsilon) &&
               nearly_equal(a.max_y, b.max_y, epsilon);
    }
};

// Construction and basic properties
TEST_F(BoundingBoxTest, DefaultConstruction) {
    BoundingBox bb;
    
    EXPECT_TRUE(bb.is_empty());
    EXPECT_FALSE(bb.is_valid());
    EXPECT_FLOAT_EQ(bb.width(), 0.0f);
    EXPECT_FLOAT_EQ(bb.height(), 0.0f);
    EXPECT_FLOAT_EQ(bb.area(), 0.0f);
    EXPECT_FLOAT_EQ(bb.perimeter(), 0.0f);
}

TEST_F(BoundingBoxTest, ParameterizedConstruction) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_FLOAT_EQ(bb.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 30.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 40.0f);
    
    EXPECT_FALSE(bb.is_empty());
    EXPECT_TRUE(bb.is_valid());
    EXPECT_FLOAT_EQ(bb.width(), 20.0f);
    EXPECT_FLOAT_EQ(bb.height(), 20.0f);
    EXPECT_FLOAT_EQ(bb.area(), 400.0f);
    EXPECT_FLOAT_EQ(bb.perimeter(), 80.0f);
}

TEST_F(BoundingBoxTest, TwoPointConstruction) {
    Point2D p1(10.0f, 20.0f);
    Point2D p2(30.0f, 40.0f);
    BoundingBox bb(p1, p2);
    
    EXPECT_FLOAT_EQ(bb.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 30.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 40.0f);
    
    // Test with reversed points
    BoundingBox bb2(p2, p1);
    EXPECT_TRUE(bb == bb2);
}

TEST_F(BoundingBoxTest, FromCenterHalfExtents) {
    Point2D center(20.0f, 30.0f);
    BoundingBox bb = BoundingBox::from_center_half_extents(center, 10.0f, 15.0f);
    
    EXPECT_FLOAT_EQ(bb.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 15.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 30.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 45.0f);
    
    // Verify center
    Point2D computed_center = bb.center();
    EXPECT_FLOAT_EQ(computed_center.x, center.x);
    EXPECT_FLOAT_EQ(computed_center.y, center.y);
}

TEST_F(BoundingBoxTest, FromCenterSize) {
    Point2D center(20.0f, 30.0f);
    BoundingBox bb = BoundingBox::from_center_size(center, 20.0f, 30.0f);
    
    EXPECT_FLOAT_EQ(bb.width(), 20.0f);
    EXPECT_FLOAT_EQ(bb.height(), 30.0f);
    
    Point2D computed_center = bb.center();
    EXPECT_FLOAT_EQ(computed_center.x, center.x);
    EXPECT_FLOAT_EQ(computed_center.y, center.y);
}

// Size and type verification
TEST_F(BoundingBoxTest, SizeAndAlignment) {
    EXPECT_EQ(sizeof(BoundingBox), 16);
    EXPECT_TRUE(std::is_trivially_copyable<BoundingBox>::value);
    EXPECT_TRUE(std::is_standard_layout<BoundingBox>::value);
}

// Corner points
TEST_F(BoundingBoxTest, CornerPoints) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    
    Point2D min_corner = bb.min_corner();
    EXPECT_FLOAT_EQ(min_corner.x, 10.0f);
    EXPECT_FLOAT_EQ(min_corner.y, 20.0f);
    
    Point2D max_corner = bb.max_corner();
    EXPECT_FLOAT_EQ(max_corner.x, 30.0f);
    EXPECT_FLOAT_EQ(max_corner.y, 40.0f);
    
    Point2D tl = bb.top_left();
    EXPECT_FLOAT_EQ(tl.x, 10.0f);
    EXPECT_FLOAT_EQ(tl.y, 40.0f);
    
    Point2D tr = bb.top_right();
    EXPECT_FLOAT_EQ(tr.x, 30.0f);
    EXPECT_FLOAT_EQ(tr.y, 40.0f);
    
    Point2D bl = bb.bottom_left();
    EXPECT_FLOAT_EQ(bl.x, 10.0f);
    EXPECT_FLOAT_EQ(bl.y, 20.0f);
    
    Point2D br = bb.bottom_right();
    EXPECT_FLOAT_EQ(br.x, 30.0f);
    EXPECT_FLOAT_EQ(br.y, 20.0f);
}

// Containment tests
TEST_F(BoundingBoxTest, ContainsPoint) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    
    // Points inside
    EXPECT_TRUE(bb.contains(Point2D(20.0f, 30.0f)));  // Center
    EXPECT_TRUE(bb.contains(Point2D(10.0f, 20.0f)));  // Min corner
    EXPECT_TRUE(bb.contains(Point2D(30.0f, 40.0f)));  // Max corner
    EXPECT_TRUE(bb.contains(Point2D(15.0f, 25.0f)));  // Inside
    
    // Points outside
    EXPECT_FALSE(bb.contains(Point2D(5.0f, 30.0f)));   // Left
    EXPECT_FALSE(bb.contains(Point2D(35.0f, 30.0f)));  // Right
    EXPECT_FALSE(bb.contains(Point2D(20.0f, 15.0f)));  // Below
    EXPECT_FALSE(bb.contains(Point2D(20.0f, 45.0f)));  // Above
    
    // Empty box contains nothing
    BoundingBox empty;
    EXPECT_FALSE(empty.contains(Point2D(0.0f, 0.0f)));
}

TEST_F(BoundingBoxTest, ContainsBoundingBox) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    
    // Box contains itself
    EXPECT_TRUE(bb.contains(bb));
    
    // Smaller box inside
    BoundingBox inside(15.0f, 25.0f, 25.0f, 35.0f);
    EXPECT_TRUE(bb.contains(inside));
    
    // Box partially outside
    BoundingBox partial(15.0f, 25.0f, 35.0f, 35.0f);
    EXPECT_FALSE(bb.contains(partial));
    
    // Box completely outside
    BoundingBox outside(40.0f, 50.0f, 60.0f, 70.0f);
    EXPECT_FALSE(bb.contains(outside));
    
    // Empty boxes
    BoundingBox empty;
    EXPECT_FALSE(bb.contains(empty));
    EXPECT_FALSE(empty.contains(bb));
}

// Intersection tests
TEST_F(BoundingBoxTest, Intersects) {
    BoundingBox bb1(10.0f, 20.0f, 30.0f, 40.0f);
    
    // Self intersection
    EXPECT_TRUE(bb1.intersects(bb1));
    
    // Overlapping boxes
    BoundingBox bb2(20.0f, 30.0f, 40.0f, 50.0f);
    EXPECT_TRUE(bb1.intersects(bb2));
    EXPECT_TRUE(bb2.intersects(bb1));
    
    // Touching boxes (edge case)
    BoundingBox bb3(30.0f, 20.0f, 50.0f, 40.0f);
    EXPECT_TRUE(bb1.intersects(bb3));
    
    // Non-intersecting boxes
    BoundingBox bb4(40.0f, 50.0f, 60.0f, 70.0f);
    EXPECT_FALSE(bb1.intersects(bb4));
    
    // Empty box
    BoundingBox empty;
    EXPECT_FALSE(bb1.intersects(empty));
    EXPECT_FALSE(empty.intersects(bb1));
}

TEST_F(BoundingBoxTest, Intersection) {
    BoundingBox bb1(10.0f, 20.0f, 30.0f, 40.0f);
    BoundingBox bb2(20.0f, 30.0f, 40.0f, 50.0f);
    
    BoundingBox result = bb1.intersection(bb2);
    EXPECT_FALSE(result.is_empty());
    EXPECT_FLOAT_EQ(result.min_x, 20.0f);
    EXPECT_FLOAT_EQ(result.min_y, 30.0f);
    EXPECT_FLOAT_EQ(result.max_x, 30.0f);
    EXPECT_FLOAT_EQ(result.max_y, 40.0f);
    
    // Non-intersecting boxes
    BoundingBox bb3(40.0f, 50.0f, 60.0f, 70.0f);
    BoundingBox empty_result = bb1.intersection(bb3);
    EXPECT_TRUE(empty_result.is_empty());
}

// Union tests
TEST_F(BoundingBoxTest, UnionWith) {
    BoundingBox bb1(10.0f, 20.0f, 30.0f, 40.0f);
    BoundingBox bb2(20.0f, 30.0f, 40.0f, 50.0f);
    
    BoundingBox result = bb1.union_with(bb2);
    EXPECT_FLOAT_EQ(result.min_x, 10.0f);
    EXPECT_FLOAT_EQ(result.min_y, 20.0f);
    EXPECT_FLOAT_EQ(result.max_x, 40.0f);
    EXPECT_FLOAT_EQ(result.max_y, 50.0f);
    
    // Union with empty
    BoundingBox empty;
    BoundingBox result2 = bb1.union_with(empty);
    EXPECT_TRUE(bb1 == result2);
    
    BoundingBox result3 = empty.union_with(bb1);
    EXPECT_TRUE(bb1 == result3);
}

// Expansion tests
TEST_F(BoundingBoxTest, ExpandPoint) {
    BoundingBox bb;
    
    // Expand empty box
    bb.expand(Point2D(10.0f, 20.0f));
    EXPECT_FALSE(bb.is_empty());
    EXPECT_FLOAT_EQ(bb.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 20.0f);
    
    // Expand existing box
    bb.expand(Point2D(30.0f, 40.0f));
    EXPECT_FLOAT_EQ(bb.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 30.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 40.0f);
    
    // Expand with point inside
    bb.expand(Point2D(20.0f, 30.0f));
    EXPECT_FLOAT_EQ(bb.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 30.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 40.0f);
}

TEST_F(BoundingBoxTest, ExpandBoundingBox) {
    BoundingBox bb1(10.0f, 20.0f, 30.0f, 40.0f);
    BoundingBox bb2(20.0f, 30.0f, 40.0f, 50.0f);
    
    bb1.expand(bb2);
    EXPECT_FLOAT_EQ(bb1.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb1.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb1.max_x, 40.0f);
    EXPECT_FLOAT_EQ(bb1.max_y, 50.0f);
    
    // Expand with empty box
    BoundingBox bb3(10.0f, 20.0f, 30.0f, 40.0f);
    BoundingBox empty;
    bb3.expand(empty);
    EXPECT_FLOAT_EQ(bb3.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb3.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb3.max_x, 30.0f);
    EXPECT_FLOAT_EQ(bb3.max_y, 40.0f);
}

// Transformation tests
TEST_F(BoundingBoxTest, Inflate) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    
    bb.inflate(5.0f);
    EXPECT_FLOAT_EQ(bb.min_x, 5.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 15.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 35.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 45.0f);
    
    // Negative inflation (deflate)
    bb.inflate(-3.0f);
    EXPECT_FLOAT_EQ(bb.min_x, 8.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 18.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 32.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 42.0f);
    
    // Empty box
    BoundingBox empty;
    empty.inflate(10.0f);
    EXPECT_TRUE(empty.is_empty());
}

TEST_F(BoundingBoxTest, InflateXY) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    
    bb.inflate(5.0f, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_x, 5.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 10.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 35.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 50.0f);
}

TEST_F(BoundingBoxTest, Scale) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    Point2D original_center = bb.center();
    
    bb.scale(2.0f);
    EXPECT_FLOAT_EQ(bb.width(), 40.0f);
    EXPECT_FLOAT_EQ(bb.height(), 40.0f);
    
    // Center should remain the same
    Point2D new_center = bb.center();
    EXPECT_FLOAT_EQ(new_center.x, original_center.x);
    EXPECT_FLOAT_EQ(new_center.y, original_center.y);
    
    // Scale down
    bb.scale(0.5f);
    EXPECT_FLOAT_EQ(bb.width(), 20.0f);
    EXPECT_FLOAT_EQ(bb.height(), 20.0f);
}

TEST_F(BoundingBoxTest, Translate) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    
    bb.translate(5.0f, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_x, 15.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 30.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 35.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 50.0f);
    
    // Translate with Point2D
    bb.translate(Point2D(-5.0f, -10.0f));
    EXPECT_FLOAT_EQ(bb.min_x, 10.0f);
    EXPECT_FLOAT_EQ(bb.min_y, 20.0f);
    EXPECT_FLOAT_EQ(bb.max_x, 30.0f);
    EXPECT_FLOAT_EQ(bb.max_y, 40.0f);
}

// Reset test
TEST_F(BoundingBoxTest, Reset) {
    BoundingBox bb(10.0f, 20.0f, 30.0f, 40.0f);
    EXPECT_FALSE(bb.is_empty());
    
    bb.reset();
    EXPECT_TRUE(bb.is_empty());
}

// Equality tests
TEST_F(BoundingBoxTest, Equality) {
    BoundingBox bb1(10.0f, 20.0f, 30.0f, 40.0f);
    BoundingBox bb2(10.0f, 20.0f, 30.0f, 40.0f);
    BoundingBox bb3(10.0f, 20.0f, 30.0f, 40.1f);
    
    EXPECT_TRUE(bb1 == bb2);
    EXPECT_FALSE(bb1 == bb3);
    EXPECT_FALSE(bb1 != bb2);
    EXPECT_TRUE(bb1 != bb3);
    
    EXPECT_TRUE(bb1.nearly_equal(bb2));
    EXPECT_FALSE(bb1.nearly_equal(bb3, 0.01f));
    EXPECT_TRUE(bb1.nearly_equal(bb3, 0.2f));
}

// Batch operations tests
class BoundingBoxBatchTest : public ::testing::Test {
protected:
    static constexpr size_t SMALL_SIZE = 8;
    static constexpr size_t MEDIUM_SIZE = 64;
    static constexpr size_t LARGE_SIZE = 1024;
    static constexpr float EPSILON = 1e-5f;
    
    std::vector<Point2D> points;
    std::vector<BoundingBox> boxes;
    std::vector<uint8_t> results;
    BoundingBox test_box;
    
    void SetUp() override {
        points.resize(LARGE_SIZE);
        boxes.resize(LARGE_SIZE);
        results.resize(LARGE_SIZE);
        
        // Create test bounding box
        test_box = BoundingBox(10.0f, 10.0f, 20.0f, 20.0f);
        
        // Fill with test data
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            // Create points both inside and outside test_box
            float x = static_cast<float>(i % 30);
            float y = static_cast<float>((i * 2) % 30);
            points[i] = Point2D(x, y);
            
            // Create overlapping and non-overlapping boxes
            float offset = static_cast<float>(i * 5);
            boxes[i] = BoundingBox(
                offset, offset,
                offset + 15.0f, offset + 15.0f
            );
        }
    }
    
    void verify_contains_points(size_t count) {
        batch::contains_points(test_box, points.data(), results.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            uint8_t expected = test_box.contains(points[i]) ? 1 : 0;
            EXPECT_EQ(results[i], expected) << "Failed at index " << i;
        }
    }
    
    void verify_intersects_boxes(size_t count) {
        batch::intersects_boxes(test_box, boxes.data(), results.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            uint8_t expected = test_box.intersects(boxes[i]) ? 1 : 0;
            EXPECT_EQ(results[i], expected) << "Failed at index " << i;
        }
    }
};

TEST_F(BoundingBoxBatchTest, BatchContainsPoints) {
    verify_contains_points(SMALL_SIZE);
    verify_contains_points(MEDIUM_SIZE);
    verify_contains_points(LARGE_SIZE);
    
    // Test with odd sizes
    verify_contains_points(7);
    verify_contains_points(63);
    verify_contains_points(1023);
}

TEST_F(BoundingBoxBatchTest, BatchIntersectsBoxes) {
    verify_intersects_boxes(SMALL_SIZE);
    verify_intersects_boxes(MEDIUM_SIZE);
    verify_intersects_boxes(LARGE_SIZE);
}

TEST_F(BoundingBoxBatchTest, BatchUnionBoxes) {
    // Test with various sizes
    std::vector<size_t> test_sizes = {0, 1, SMALL_SIZE, MEDIUM_SIZE, LARGE_SIZE};
    for (size_t count : test_sizes) {
        BoundingBox result = batch::union_boxes(boxes.data(), count);
        
        if (count == 0) {
            EXPECT_TRUE(result.is_empty());
        } else {
            // Verify manually
            BoundingBox expected = boxes[0];
            for (size_t i = 1; i < count; ++i) {
                expected.expand(boxes[i]);
            }
            
            EXPECT_NEAR(result.min_x, expected.min_x, EPSILON);
            EXPECT_NEAR(result.min_y, expected.min_y, EPSILON);
            EXPECT_NEAR(result.max_x, expected.max_x, EPSILON);
            EXPECT_NEAR(result.max_y, expected.max_y, EPSILON);
        }
    }
}

TEST_F(BoundingBoxBatchTest, BatchInflateBoxes) {
    std::vector<BoundingBox> inflated(boxes.size());
    float amount = 5.0f;
    
    std::vector<size_t> batch_sizes = {SMALL_SIZE, MEDIUM_SIZE, LARGE_SIZE};
    for (size_t count : batch_sizes) {
        batch::inflate_boxes(boxes.data(), inflated.data(), amount, count);
        
        for (size_t i = 0; i < count; ++i) {
            BoundingBox expected = boxes[i];
            expected.inflate(amount);
            
            EXPECT_NEAR(inflated[i].min_x, expected.min_x, EPSILON);
            EXPECT_NEAR(inflated[i].min_y, expected.min_y, EPSILON);
            EXPECT_NEAR(inflated[i].max_x, expected.max_x, EPSILON);
            EXPECT_NEAR(inflated[i].max_y, expected.max_y, EPSILON);
        }
    }
}

TEST_F(BoundingBoxBatchTest, BatchEmptyInput) {
    // Test with count = 0
    batch::contains_points(test_box, points.data(), results.data(), 0);
    batch::intersects_boxes(test_box, boxes.data(), results.data(), 0);
    
    BoundingBox empty_union = batch::union_boxes(nullptr, 0);
    EXPECT_TRUE(empty_union.is_empty());
    
    batch::inflate_boxes(boxes.data(), boxes.data(), 5.0f, 0);
}

// Edge cases
TEST_F(BoundingBoxTest, EdgeCases) {
    // Degenerate box (zero area)
    BoundingBox line_h(10.0f, 20.0f, 30.0f, 20.0f);
    EXPECT_FALSE(line_h.is_empty());
    EXPECT_FLOAT_EQ(line_h.area(), 0.0f);
    EXPECT_FLOAT_EQ(line_h.height(), 0.0f);
    EXPECT_FLOAT_EQ(line_h.width(), 20.0f);
    
    BoundingBox line_v(10.0f, 20.0f, 10.0f, 40.0f);
    EXPECT_FALSE(line_v.is_empty());
    EXPECT_FLOAT_EQ(line_v.area(), 0.0f);
    EXPECT_FLOAT_EQ(line_v.width(), 0.0f);
    EXPECT_FLOAT_EQ(line_v.height(), 20.0f);
    
    // Point box
    BoundingBox point(10.0f, 20.0f, 10.0f, 20.0f);
    EXPECT_FALSE(point.is_empty());
    EXPECT_FLOAT_EQ(point.area(), 0.0f);
    EXPECT_TRUE(point.contains(Point2D(10.0f, 20.0f)));
    
    // Very large values
    BoundingBox large(-1e10f, -1e10f, 1e10f, 1e10f);
    EXPECT_FALSE(large.is_empty());
    EXPECT_TRUE(large.contains(Point2D(0.0f, 0.0f)));
    
    // Negative size (invalid)
    BoundingBox invalid(30.0f, 40.0f, 10.0f, 20.0f);
    EXPECT_TRUE(invalid.is_empty());
}