#include <gtest/gtest.h>
#include "claude_draw/core/color.h"
#include "claude_draw/core/simd.h"
#include "../test_utils.h"
#include <vector>
#include <cmath>
#include <limits>

using namespace claude_draw;

class ColorTest : public ::testing::Test {
protected:
    static constexpr float EPSILON = 1.0f / 255.0f;  // One unit in byte precision
    
    bool nearly_equal(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
    
    bool colors_equal(const Color& a, const Color& b, int tolerance = 1) {
        return std::abs(a.r - b.r) <= tolerance &&
               std::abs(a.g - b.g) <= tolerance &&
               std::abs(a.b - b.b) <= tolerance &&
               std::abs(a.a - b.a) <= tolerance;
    }
};

// Construction and basic properties
TEST_F(ColorTest, DefaultConstruction) {
    Color c;
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
    EXPECT_EQ(c.a, 255);
    EXPECT_EQ(c.rgba, 0xFF000000);
}

TEST_F(ColorTest, PackedConstruction) {
    Color c(0x12345678);
    EXPECT_EQ(c.rgba, 0x12345678);
    EXPECT_EQ(c.r, 0x78);
    EXPECT_EQ(c.g, 0x56);
    EXPECT_EQ(c.b, 0x34);
    EXPECT_EQ(c.a, 0x12);
}

TEST_F(ColorTest, ComponentConstruction) {
    Color c(static_cast<uint8_t>(10), static_cast<uint8_t>(20), static_cast<uint8_t>(30), static_cast<uint8_t>(40));
    EXPECT_EQ(c.r, 10);
    EXPECT_EQ(c.g, 20);
    EXPECT_EQ(c.b, 30);
    EXPECT_EQ(c.a, 40);
}

TEST_F(ColorTest, ComponentConstructionDefaultAlpha) {
    Color c(static_cast<uint8_t>(100), static_cast<uint8_t>(150), static_cast<uint8_t>(200));
    EXPECT_EQ(c.r, 100);
    EXPECT_EQ(c.g, 150);
    EXPECT_EQ(c.b, 200);
    EXPECT_EQ(c.a, 255);
}

TEST_F(ColorTest, FloatConstruction) {
    Color c(0.5f, 0.25f, 0.75f, 1.0f);
    EXPECT_EQ(c.r, 128);  // 0.5 * 255 + 0.5 = 128
    EXPECT_EQ(c.g, 64);   // 0.25 * 255 + 0.5 = 64
    EXPECT_EQ(c.b, 191);  // 0.75 * 255 + 0.5 = 191
    EXPECT_EQ(c.a, 255);
}

TEST_F(ColorTest, FloatConstructionClamping) {
    Color c1(-0.5f, 1.5f, 0.0f, 2.0f);
    EXPECT_EQ(c1.r, 0);
    EXPECT_EQ(c1.g, 255);
    EXPECT_EQ(c1.b, 0);
    EXPECT_EQ(c1.a, 255);
}

// Size verification
TEST_F(ColorTest, SizeVerification) {
    EXPECT_EQ(sizeof(Color), 4);
    EXPECT_EQ(offsetof(Color, r), 0);
    EXPECT_EQ(offsetof(Color, g), 1);
    EXPECT_EQ(offsetof(Color, b), 2);
    EXPECT_EQ(offsetof(Color, a), 3);
}

// Float conversions
TEST_F(ColorTest, FloatGetters) {
    Color c(static_cast<uint8_t>(51), static_cast<uint8_t>(102), static_cast<uint8_t>(153), static_cast<uint8_t>(204));  // 0.2, 0.4, 0.6, 0.8
    EXPECT_NEAR(c.red_f(), 0.2f, EPSILON);
    EXPECT_NEAR(c.green_f(), 0.4f, EPSILON);
    EXPECT_NEAR(c.blue_f(), 0.6f, EPSILON);
    EXPECT_NEAR(c.alpha_f(), 0.8f, EPSILON);
}

TEST_F(ColorTest, FloatSetters) {
    Color c;
    c.set_red_f(0.2f);
    c.set_green_f(0.4f);
    c.set_blue_f(0.6f);
    c.set_alpha_f(0.8f);
    
    EXPECT_EQ(c.r, 51);
    EXPECT_EQ(c.g, 102);
    EXPECT_EQ(c.b, 153);
    EXPECT_EQ(c.a, 204);
}

// Equality operators
TEST_F(ColorTest, Equality) {
    Color c1(static_cast<uint8_t>(10), static_cast<uint8_t>(20), static_cast<uint8_t>(30), static_cast<uint8_t>(40));
    Color c2(static_cast<uint8_t>(10), static_cast<uint8_t>(20), static_cast<uint8_t>(30), static_cast<uint8_t>(40));
    Color c3(static_cast<uint8_t>(10), static_cast<uint8_t>(20), static_cast<uint8_t>(30), static_cast<uint8_t>(41));
    
    EXPECT_TRUE(c1 == c2);
    EXPECT_FALSE(c1 == c3);
    EXPECT_FALSE(c1 != c2);
    EXPECT_TRUE(c1 != c3);
}

// Alpha blending
TEST_F(ColorTest, BlendOverOpaque) {
    Color src(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));  // Opaque red
    Color dst(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(255));  // Opaque green
    Color result = src.blend_over(dst);
    
    EXPECT_EQ(result.r, 255);
    EXPECT_EQ(result.g, 0);
    EXPECT_EQ(result.b, 0);
    EXPECT_EQ(result.a, 255);
}

TEST_F(ColorTest, BlendOverTransparent) {
    Color src(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(0));    // Transparent red
    Color dst(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(255));  // Opaque green
    Color result = src.blend_over(dst);
    
    EXPECT_EQ(result.r, 0);
    EXPECT_EQ(result.g, 255);
    EXPECT_EQ(result.b, 0);
    EXPECT_EQ(result.a, 255);
}

TEST_F(ColorTest, BlendOverSemiTransparent) {
    Color src(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(128));  // 50% red
    Color dst(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(255));  // Opaque green
    Color result = src.blend_over(dst);
    
    // Should be roughly 50% red, 50% green
    EXPECT_TRUE(colors_equal(result, Color(static_cast<uint8_t>(128), static_cast<uint8_t>(127), static_cast<uint8_t>(0), static_cast<uint8_t>(255)), 2));
}

// Premultiply alpha
TEST_F(ColorTest, PremultiplyOpaque) {
    Color c(static_cast<uint8_t>(100), static_cast<uint8_t>(150), static_cast<uint8_t>(200), static_cast<uint8_t>(255));
    Color result = c.premultiply();
    
    EXPECT_EQ(result.r, 100);
    EXPECT_EQ(result.g, 150);
    EXPECT_EQ(result.b, 200);
    EXPECT_EQ(result.a, 255);
}

TEST_F(ColorTest, PremultiplyTransparent) {
    Color c(static_cast<uint8_t>(100), static_cast<uint8_t>(150), static_cast<uint8_t>(200), static_cast<uint8_t>(0));
    Color result = c.premultiply();
    
    EXPECT_EQ(result.r, 0);
    EXPECT_EQ(result.g, 0);
    EXPECT_EQ(result.b, 0);
    EXPECT_EQ(result.a, 0);
}

TEST_F(ColorTest, PremultiplySemiTransparent) {
    Color c(static_cast<uint8_t>(100), static_cast<uint8_t>(150), static_cast<uint8_t>(200), static_cast<uint8_t>(128));
    Color result = c.premultiply();
    
    EXPECT_NEAR(result.r, 50, 1);
    EXPECT_NEAR(result.g, 75, 1);
    EXPECT_NEAR(result.b, 100, 1);
    EXPECT_EQ(result.a, 128);
}

// Hex conversions
TEST_F(ColorTest, ToHex) {
    Color c(static_cast<uint8_t>(0x12), static_cast<uint8_t>(0x34), static_cast<uint8_t>(0x56), static_cast<uint8_t>(0x78));
    EXPECT_EQ(c.to_hex(), 0x123456);
}

TEST_F(ColorTest, FromHex) {
    Color c = Color::from_hex(0x123456);
    EXPECT_EQ(c.r, 0x12);
    EXPECT_EQ(c.g, 0x34);
    EXPECT_EQ(c.b, 0x56);
    EXPECT_EQ(c.a, 255);
    
    Color c2 = Color::from_hex(0xABCDEF, static_cast<uint8_t>(128));
    EXPECT_EQ(c2.r, 0xAB);
    EXPECT_EQ(c2.g, 0xCD);
    EXPECT_EQ(c2.b, 0xEF);
    EXPECT_EQ(c2.a, 128);
}

// HSL conversions
TEST_F(ColorTest, ToHSLGrayscale) {
    Color gray(static_cast<uint8_t>(128), static_cast<uint8_t>(128), static_cast<uint8_t>(128));
    float h, s, l;
    gray.to_hsl(h, s, l);
    
    EXPECT_NEAR(h, 0.0f, EPSILON);
    EXPECT_NEAR(s, 0.0f, EPSILON);
    EXPECT_NEAR(l, 0.5f, EPSILON * 2);
}

TEST_F(ColorTest, ToHSLPureRed) {
    Color red(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0));
    float h, s, l;
    red.to_hsl(h, s, l);
    
    EXPECT_NEAR(h, 0.0f, EPSILON);
    EXPECT_NEAR(s, 1.0f, EPSILON);
    EXPECT_NEAR(l, 0.5f, EPSILON);
}

TEST_F(ColorTest, ToHSLPureGreen) {
    Color green(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0));
    float h, s, l;
    green.to_hsl(h, s, l);
    
    EXPECT_NEAR(h, 1.0f/3.0f, EPSILON * 2);
    EXPECT_NEAR(s, 1.0f, EPSILON);
    EXPECT_NEAR(l, 0.5f, EPSILON);
}

TEST_F(ColorTest, ToHSLPureBlue) {
    Color blue(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    float h, s, l;
    blue.to_hsl(h, s, l);
    
    EXPECT_NEAR(h, 2.0f/3.0f, EPSILON * 2);
    EXPECT_NEAR(s, 1.0f, EPSILON);
    EXPECT_NEAR(l, 0.5f, EPSILON);
}

TEST_F(ColorTest, FromHSL) {
    // Test grayscale
    Color gray = Color::from_hsl(0.0f, 0.0f, 0.5f);
    EXPECT_TRUE(colors_equal(gray, Color(static_cast<uint8_t>(128), static_cast<uint8_t>(128), static_cast<uint8_t>(128)), 1));
    
    // Test pure colors
    Color red = Color::from_hsl(0.0f, 1.0f, 0.5f);
    EXPECT_TRUE(colors_equal(red, Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0)), 1));
    
    Color green = Color::from_hsl(1.0f/3.0f, 1.0f, 0.5f);
    EXPECT_TRUE(colors_equal(green, Color(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0)), 1));
    
    Color blue = Color::from_hsl(2.0f/3.0f, 1.0f, 0.5f);
    EXPECT_TRUE(colors_equal(blue, Color(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255)), 1));
}

TEST_F(ColorTest, HSLRoundTrip) {
    // Test various colors for HSL round-trip accuracy
    std::vector<Color> test_colors = {
        Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0)),      // Red
        Color(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0)),      // Green
        Color(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255)),      // Blue
        Color(static_cast<uint8_t>(255), static_cast<uint8_t>(255), static_cast<uint8_t>(0)),    // Yellow
        Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(255)),    // Magenta
        Color(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(255)),    // Cyan
        Color(static_cast<uint8_t>(128), static_cast<uint8_t>(64), static_cast<uint8_t>(192)),   // Purple-ish
        Color(static_cast<uint8_t>(255), static_cast<uint8_t>(128), static_cast<uint8_t>(64)),   // Orange-ish
    };
    
    for (const auto& original : test_colors) {
        float h, s, l;
        original.to_hsl(h, s, l);
        Color converted = Color::from_hsl(h, s, l, original.a);
        
        EXPECT_TRUE(colors_equal(original, converted, 2)) 
            << "Original: " << original << ", Converted: " << converted;
    }
}

// Color constants
TEST_F(ColorTest, ColorConstants) {
    EXPECT_EQ(Color::transparent().rgba, 0x00000000);
    EXPECT_EQ(Color::black().rgba, 0xFF000000);
    EXPECT_EQ(Color::white().rgba, 0xFFFFFFFF);
    EXPECT_EQ(Color::red(), Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(0)));
    EXPECT_EQ(Color::green(), Color(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(0)));
    EXPECT_EQ(Color::blue(), Color(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
    EXPECT_EQ(Color::yellow(), Color(static_cast<uint8_t>(255), static_cast<uint8_t>(255), static_cast<uint8_t>(0)));
    EXPECT_EQ(Color::cyan(), Color(static_cast<uint8_t>(0), static_cast<uint8_t>(255), static_cast<uint8_t>(255)));
    EXPECT_EQ(Color::magenta(), Color(static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(255)));
}

// Batch operations tests
class ColorBatchTest : public ::testing::Test {
protected:
    static constexpr size_t SMALL_SIZE = 8;
    static constexpr size_t MEDIUM_SIZE = 64;
    static constexpr size_t LARGE_SIZE = 1024;
    
    std::vector<Color> src, dst, result;
    
    void SetUp() override {
        src.resize(LARGE_SIZE);
        dst.resize(LARGE_SIZE);
        result.resize(LARGE_SIZE);
        
        // Fill with test data
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            src[i] = Color(
                static_cast<uint8_t>(i % 256),
                static_cast<uint8_t>((i * 2) % 256),
                static_cast<uint8_t>((i * 3) % 256),
                static_cast<uint8_t>((i * 4) % 256)
            );
            dst[i] = Color(
                static_cast<uint8_t>((255 - i) % 256),
                static_cast<uint8_t>((255 - i * 2) % 256),
                static_cast<uint8_t>((255 - i * 3) % 256),
                static_cast<uint8_t>(255)
            );
        }
    }
    
    void verify_batch_blend(size_t count) {
        batch::blend_over(src.data(), dst.data(), result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            Color expected = src[i].blend_over(dst[i]);
            EXPECT_TRUE(colors_equal(result[i], expected, 2))
                << "Failed at index " << i 
                << ", expected: " << expected
                << ", got: " << result[i];
        }
    }
    
    void verify_batch_premultiply(size_t count) {
        batch::premultiply(src.data(), result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            Color expected = src[i].premultiply();
            EXPECT_TRUE(colors_equal(result[i], expected, 2))
                << "Failed at index " << i;
        }
    }
    
    void verify_batch_grayscale(size_t count) {
        batch::to_grayscale(src.data(), result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            uint8_t gray = static_cast<uint8_t>(
                0.299f * src[i].r + 
                0.587f * src[i].g + 
                0.114f * src[i].b + 0.5f
            );
            Color expected(static_cast<uint8_t>(gray), static_cast<uint8_t>(gray), static_cast<uint8_t>(gray), src[i].a);
            EXPECT_TRUE(colors_equal(result[i], expected, 2))
                << "Failed at index " << i;
        }
    }
    
    bool colors_equal(const Color& a, const Color& b, int tolerance = 1) {
        return std::abs(a.r - b.r) <= tolerance &&
               std::abs(a.g - b.g) <= tolerance &&
               std::abs(a.b - b.b) <= tolerance &&
               std::abs(a.a - b.a) <= tolerance;
    }
};

TEST_F(ColorBatchTest, BatchBlendOver) {
    verify_batch_blend(SMALL_SIZE);
    verify_batch_blend(MEDIUM_SIZE);
    verify_batch_blend(LARGE_SIZE);
    
    // Test with odd sizes
    verify_batch_blend(7);
    verify_batch_blend(63);
    verify_batch_blend(1023);
}

TEST_F(ColorBatchTest, BatchPremultiply) {
    verify_batch_premultiply(SMALL_SIZE);
    verify_batch_premultiply(MEDIUM_SIZE);
    verify_batch_premultiply(LARGE_SIZE);
    
    verify_batch_premultiply(7);
    verify_batch_premultiply(63);
    verify_batch_premultiply(1023);
}

TEST_F(ColorBatchTest, BatchToGrayscale) {
    verify_batch_grayscale(SMALL_SIZE);
    verify_batch_grayscale(MEDIUM_SIZE);
    verify_batch_grayscale(LARGE_SIZE);
    
    verify_batch_grayscale(7);
    verify_batch_grayscale(63);
    verify_batch_grayscale(1023);
}

TEST_F(ColorBatchTest, BatchEmptyInput) {
    // Test with count = 0
    batch::blend_over(src.data(), dst.data(), result.data(), 0);
    batch::premultiply(src.data(), result.data(), 0);
    batch::to_grayscale(src.data(), result.data(), 0);
    // Should not crash
}

TEST_F(ColorBatchTest, BatchSingleElement) {
    verify_batch_blend(1);
    verify_batch_premultiply(1);
    verify_batch_grayscale(1);
}