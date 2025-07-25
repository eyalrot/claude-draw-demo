#include <gtest/gtest.h>
#include "claude_draw/shapes/shape_base_optimized.h"
#include <cstddef>
#include <type_traits>
#include <chrono>
#include <iostream>

using namespace claude_draw::shapes;

// Test suite for shape memory layout verification
class ShapeLayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed
    }
};

// Test CircleShape layout
TEST_F(ShapeLayoutTest, CircleShapeLayout) {
    // Verify size is exactly 32 bytes
    EXPECT_EQ(sizeof(CircleShape), 32u);
    
    // Verify alignment is 32 bytes
    EXPECT_EQ(alignof(CircleShape), 32u);
    
    // Verify member offsets for cache line optimization
    EXPECT_EQ(offsetof(CircleShape, center_x), 0u);
    EXPECT_EQ(offsetof(CircleShape, center_y), 4u);
    EXPECT_EQ(offsetof(CircleShape, radius), 8u);
    EXPECT_EQ(offsetof(CircleShape, fill_color), 12u);
    EXPECT_EQ(offsetof(CircleShape, stroke_color), 16u);
    EXPECT_EQ(offsetof(CircleShape, stroke_width), 20u);
    EXPECT_EQ(offsetof(CircleShape, type), 24u);
    EXPECT_EQ(offsetof(CircleShape, flags), 25u);
    EXPECT_EQ(offsetof(CircleShape, reserved), 26u);
    EXPECT_EQ(offsetof(CircleShape, id), 28u);
    
    // Verify no unexpected padding
    size_t expected_size = sizeof(float) * 3 +     // center_x, center_y, radius
                          sizeof(uint32_t) * 2 +   // fill_color, stroke_color
                          sizeof(float) +          // stroke_width
                          sizeof(ShapeType) +      // type
                          sizeof(ShapeFlags) +     // flags
                          sizeof(uint16_t) +       // reserved
                          sizeof(uint32_t);        // id
    EXPECT_EQ(expected_size, 32u);
}

// Test RectangleShape layout
TEST_F(ShapeLayoutTest, RectangleShapeLayout) {
    // Verify size is exactly 32 bytes
    EXPECT_EQ(sizeof(RectangleShape), 32u);
    
    // Verify alignment is 32 bytes
    EXPECT_EQ(alignof(RectangleShape), 32u);
    
    // Verify member offsets
    EXPECT_EQ(offsetof(RectangleShape, x1), 0u);
    EXPECT_EQ(offsetof(RectangleShape, y1), 4u);
    EXPECT_EQ(offsetof(RectangleShape, x2), 8u);
    EXPECT_EQ(offsetof(RectangleShape, y2), 12u);
    EXPECT_EQ(offsetof(RectangleShape, fill_color), 16u);
    EXPECT_EQ(offsetof(RectangleShape, stroke_color), 20u);
    EXPECT_EQ(offsetof(RectangleShape, stroke_width), 24u);
    EXPECT_EQ(offsetof(RectangleShape, type), 28u);
    EXPECT_EQ(offsetof(RectangleShape, flags), 29u);
    EXPECT_EQ(offsetof(RectangleShape, id), 30u);
    
    // Verify frequently accessed geometry data is in first 16 bytes
    EXPECT_LE(offsetof(RectangleShape, y2), 16u);
}

// Test EllipseShape layout
TEST_F(ShapeLayoutTest, EllipseShapeLayout) {
    // Verify size is exactly 32 bytes
    EXPECT_EQ(sizeof(EllipseShape), 32u);
    
    // Verify alignment is 32 bytes
    EXPECT_EQ(alignof(EllipseShape), 32u);
    
    // Verify member offsets
    EXPECT_EQ(offsetof(EllipseShape, center_x), 0u);
    EXPECT_EQ(offsetof(EllipseShape, center_y), 4u);
    EXPECT_EQ(offsetof(EllipseShape, radius_x), 8u);
    EXPECT_EQ(offsetof(EllipseShape, radius_y), 12u);
    
    // Geometry data should be in first 16 bytes
    EXPECT_LE(offsetof(EllipseShape, radius_y) + sizeof(float), 16u);
}

// Test LineShape layout
TEST_F(ShapeLayoutTest, LineShapeLayout) {
    // Verify size is exactly 32 bytes
    EXPECT_EQ(sizeof(LineShape), 32u);
    
    // Verify alignment is 32 bytes
    EXPECT_EQ(alignof(LineShape), 32u);
    
    // Verify member offsets
    EXPECT_EQ(offsetof(LineShape, x1), 0u);
    EXPECT_EQ(offsetof(LineShape, y1), 4u);
    EXPECT_EQ(offsetof(LineShape, x2), 8u);
    EXPECT_EQ(offsetof(LineShape, y2), 12u);
    
    // Verify line endpoints are in first 16 bytes for cache efficiency
    EXPECT_LE(offsetof(LineShape, y2) + sizeof(float), 16u);
}

// Test ShapeUnion layout
TEST_F(ShapeLayoutTest, ShapeUnionLayout) {
    // Verify size is exactly 32 bytes
    EXPECT_EQ(sizeof(ShapeUnion), 32u);
    
    // Verify alignment is 32 bytes
    EXPECT_EQ(alignof(ShapeUnion), 32u);
    
    // Verify all shape types fit in the union
    EXPECT_EQ(sizeof(ShapeUnion::circle), sizeof(ShapeUnion));
    EXPECT_EQ(sizeof(ShapeUnion::rectangle), sizeof(ShapeUnion));
    EXPECT_EQ(sizeof(ShapeUnion::ellipse), sizeof(ShapeUnion));
    EXPECT_EQ(sizeof(ShapeUnion::line), sizeof(ShapeUnion));
}

// Test cache line alignment
TEST_F(ShapeLayoutTest, CacheLineAlignment) {
    constexpr size_t CACHE_LINE_SIZE = 64;
    
    // Two shapes should fit exactly in one cache line
    EXPECT_EQ(sizeof(CircleShape) * 2, CACHE_LINE_SIZE);
    EXPECT_EQ(sizeof(RectangleShape) * 2, CACHE_LINE_SIZE);
    
    // Array of shapes should maintain alignment
    CircleShape circles[4];
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&circles[0]) % 32, 0u);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&circles[1]) % 32, 0u);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&circles[2]) % 32, 0u);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&circles[3]) % 32, 0u);
}

// Test that shapes are trivially copyable for performance
TEST_F(ShapeLayoutTest, TriviallyCopyable) {
    EXPECT_TRUE(std::is_trivially_copyable<CircleShape>::value);
    EXPECT_TRUE(std::is_trivially_copyable<RectangleShape>::value);
    EXPECT_TRUE(std::is_trivially_copyable<EllipseShape>::value);
    EXPECT_TRUE(std::is_trivially_copyable<LineShape>::value);
    EXPECT_TRUE(std::is_trivially_copyable<ShapeUnion>::value);
}

// Test POD (Plain Old Data) properties for C compatibility
TEST_F(ShapeLayoutTest, PODTypes) {
    EXPECT_TRUE(std::is_standard_layout<CircleShape>::value);
    EXPECT_TRUE(std::is_standard_layout<RectangleShape>::value);
    EXPECT_TRUE(std::is_standard_layout<EllipseShape>::value);
    EXPECT_TRUE(std::is_standard_layout<LineShape>::value);
}

// Test memory access patterns
TEST_F(ShapeLayoutTest, MemoryAccessPatterns) {
    // Create array of shapes
    constexpr size_t NUM_SHAPES = 1000;
    std::unique_ptr<CircleShape[]> circles(new CircleShape[NUM_SHAPES]);
    
    // Verify array maintains alignment
    for (size_t i = 0; i < NUM_SHAPES; ++i) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(&circles[i]);
        EXPECT_EQ(addr % 32, 0u) << "Shape at index " << i << " is not 32-byte aligned";
    }
    
    // Verify stride is exactly 32 bytes
    for (size_t i = 1; i < NUM_SHAPES; ++i) {
        uintptr_t addr1 = reinterpret_cast<uintptr_t>(&circles[i-1]);
        uintptr_t addr2 = reinterpret_cast<uintptr_t>(&circles[i]);
        EXPECT_EQ(addr2 - addr1, 32u) << "Incorrect stride between shapes";
    }
}

// Test that shape type can be quickly determined
TEST_F(ShapeLayoutTest, TypeIdentification) {
    CircleShape circle;
    RectangleShape rect;
    EllipseShape ellipse;
    LineShape line;
    
    EXPECT_EQ(circle.type, ShapeType::Circle);
    EXPECT_EQ(rect.type, static_cast<uint8_t>(ShapeType::Rectangle));
    EXPECT_EQ(ellipse.type, static_cast<uint8_t>(ShapeType::Ellipse));
    EXPECT_EQ(line.type, static_cast<uint8_t>(ShapeType::Line));
    
    // Type field should be at same offset for all shapes
    EXPECT_EQ(offsetof(CircleShape, type), 24u);
    EXPECT_EQ(offsetof(RectangleShape, type), 28u);
    EXPECT_EQ(offsetof(EllipseShape, type), 28u);
    EXPECT_EQ(offsetof(LineShape, type), 28u);
}

// Static assertions for compile-time verification
static_assert(sizeof(CircleShape) == 32, "CircleShape must be 32 bytes");
static_assert(sizeof(RectangleShape) == 32, "RectangleShape must be 32 bytes");
static_assert(sizeof(EllipseShape) == 32, "EllipseShape must be 32 bytes");
static_assert(sizeof(LineShape) == 32, "LineShape must be 32 bytes");
static_assert(sizeof(ShapeUnion) == 32, "ShapeUnion must be 32 bytes");

static_assert(alignof(CircleShape) == 32, "CircleShape must be 32-byte aligned");
static_assert(alignof(RectangleShape) == 32, "RectangleShape must be 32-byte aligned");
static_assert(alignof(EllipseShape) == 32, "EllipseShape must be 32-byte aligned");
static_assert(alignof(LineShape) == 32, "LineShape must be 32-byte aligned");
static_assert(alignof(ShapeUnion) == 32, "ShapeUnion must be 32-byte aligned");



