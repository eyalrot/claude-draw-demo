#include <gtest/gtest.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>
#include <chrono>
#include <vector>

#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"

namespace py = pybind11;
using namespace claude_draw;

// Test fixture for Python bindings
class ConversionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Python interpreter once
        static py::scoped_interpreter guard{};
    }
};

// Helper to measure conversion time
template<typename F>
double measure_time_ms(F&& func, int iterations = 1000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return static_cast<double>(duration.count()) / iterations / 1000.0;  // ms per operation
}

TEST_F(ConversionTest, Point2DTupleConversion) {
    // Test tuple -> Point2D conversion
    py::tuple t = py::make_tuple(1.5f, 2.5f);
    Point2D p = t.cast<Point2D>();
    EXPECT_FLOAT_EQ(p.x, 1.5f);
    EXPECT_FLOAT_EQ(p.y, 2.5f);
    
    // Test Point2D -> tuple conversion
    Point2D p2(3.0f, 4.0f);
    py::tuple t2 = py::cast(p2);
    EXPECT_EQ(t2.size(), 2);
    EXPECT_FLOAT_EQ(t2[0].cast<float>(), 3.0f);
    EXPECT_FLOAT_EQ(t2[1].cast<float>(), 4.0f);
}

TEST_F(ConversionTest, Point2DListConversion) {
    // Test list -> Point2D conversion
    py::list lst;
    lst.append(5.0f);
    lst.append(6.0f);
    Point2D p = lst.cast<Point2D>();
    EXPECT_FLOAT_EQ(p.x, 5.0f);
    EXPECT_FLOAT_EQ(p.y, 6.0f);
}

TEST_F(ConversionTest, Point2DNumpyConversion) {
    // Create numpy array
    py::array_t<float> arr({2});
    auto ptr = static_cast<float*>(arr.mutable_unchecked<1>().mutable_data(0));
    ptr[0] = 7.0f;
    ptr[1] = 8.0f;
    
    // Test numpy -> Point2D conversion
    Point2D p = arr.cast<Point2D>();
    EXPECT_FLOAT_EQ(p.x, 7.0f);
    EXPECT_FLOAT_EQ(p.y, 8.0f);
}

TEST_F(ConversionTest, Point2DObjectConversion) {
    // Create a Python object with x, y attributes
    py::module_ builtins = py::module_::import("builtins");
    py::object obj = builtins.attr("type")("Point", py::tuple(), py::dict());
    obj.attr("x") = 9.0f;
    obj.attr("y") = 10.0f;
    
    Point2D p = obj.cast<Point2D>();
    EXPECT_FLOAT_EQ(p.x, 9.0f);
    EXPECT_FLOAT_EQ(p.y, 10.0f);
}

TEST_F(ConversionTest, ColorIntegerConversion) {
    // Test integer -> Color conversion
    py::int_ rgba = 0xFF0080FF;  // Red=255, Green=0, Blue=128, Alpha=255
    Color c = rgba.cast<Color>();
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 128);
    EXPECT_EQ(c.a, 255);
    
    // Test Color -> integer conversion
    Color c2(64, 128, 192, 255);
    py::int_ rgba2 = py::cast(c2);
    uint32_t val = rgba2.cast<uint32_t>();
    EXPECT_EQ(val, c2.rgba);
}

TEST_F(ConversionTest, ColorTupleConversion) {
    // Test tuple (int) -> Color conversion
    py::tuple t = py::make_tuple(255, 128, 64, 192);
    Color c = t.cast<Color>();
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 128);
    EXPECT_EQ(c.b, 64);
    EXPECT_EQ(c.a, 192);
    
    // Test tuple (float) -> Color conversion
    py::tuple tf = py::make_tuple(1.0f, 0.5f, 0.25f, 0.75f);
    Color cf = tf.cast<Color>();
    EXPECT_EQ(cf.r, 255);
    EXPECT_EQ(cf.g, 127);  // 0.5 * 255 = 127.5, rounded down
    EXPECT_EQ(cf.b, 63);   // 0.25 * 255 = 63.75, rounded down
    EXPECT_EQ(cf.a, 191);  // 0.75 * 255 = 191.25, rounded down
}

TEST_F(ConversionTest, ColorObjectConversion) {
    // Create a Python object with r, g, b, a attributes (int)
    py::module_ builtins = py::module_::import("builtins");
    py::object obj = builtins.attr("type")("Color", py::tuple(), py::dict());
    obj.attr("r") = 100;
    obj.attr("g") = 150;
    obj.attr("b") = 200;
    obj.attr("a") = 250;
    
    Color c = obj.cast<Color>();
    EXPECT_EQ(c.r, 100);
    EXPECT_EQ(c.g, 150);
    EXPECT_EQ(c.b, 200);
    EXPECT_EQ(c.a, 250);
    
    // Test with float attributes
    py::object objf = builtins.attr("type")("ColorFloat", py::tuple(), py::dict());
    objf.attr("r") = 0.4f;
    objf.attr("g") = 0.6f;
    objf.attr("b") = 0.8f;
    objf.attr("a") = 1.0f;
    
    Color cf = objf.cast<Color>();
    EXPECT_EQ(cf.r, 102);  // 0.4 * 255 = 102
    EXPECT_EQ(cf.g, 153);  // 0.6 * 255 = 153
    EXPECT_EQ(cf.b, 204);  // 0.8 * 255 = 204
    EXPECT_EQ(cf.a, 255);
}

TEST_F(ConversionTest, BatchConversionPerformance) {
    const int count = 10000;
    
    // Test Point2D batch conversion performance
    {
        std::vector<py::tuple> tuples;
        tuples.reserve(count);
        for (int i = 0; i < count; ++i) {
            tuples.push_back(py::make_tuple(float(i), float(i + 1)));
        }
        
        std::vector<Point2D> points;
        points.reserve(count);
        
        double time_ms = measure_time_ms([&]() {
            points.clear();
            for (const auto& t : tuples) {
                points.push_back(t.cast<Point2D>());
            }
        }, 100);
        
        std::cout << "Point2D tuple conversion: " << time_ms << " ms for " << count << " points" << std::endl;
        std::cout << "  Per-point time: " << (time_ms * 1000 / count) << " µs" << std::endl;
        
        // We expect sub-millisecond conversion time for 10k points
        EXPECT_LT(time_ms, 1.0);
    }
    
    // Test Color batch conversion performance
    {
        std::vector<py::int_> ints;
        ints.reserve(count);
        for (int i = 0; i < count; ++i) {
            ints.push_back(py::int_(0xFF000000 | (i & 0xFFFFFF)));
        }
        
        std::vector<Color> colors;
        colors.reserve(count);
        
        double time_ms = measure_time_ms([&]() {
            colors.clear();
            for (const auto& i : ints) {
                colors.push_back(i.cast<Color>());
            }
        }, 100);
        
        std::cout << "Color int conversion: " << time_ms << " ms for " << count << " colors" << std::endl;
        std::cout << "  Per-color time: " << (time_ms * 1000 / count) << " µs" << std::endl;
        
        // We expect sub-millisecond conversion time for 10k colors
        EXPECT_LT(time_ms, 1.0);
    }
}

TEST_F(ConversionTest, ZeroCopyNumpyArrays) {
    // Test zero-copy numpy array creation from Point2D data
    const size_t count = 1000;
    std::vector<Point2D> points;
    points.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        points.emplace_back(float(i), float(i * 2));
    }
    
    // Create numpy view without copying
    py::array_t<float> arr({count, size_t(2)}, {sizeof(Point2D), sizeof(float)}, 
                           reinterpret_cast<float*>(points.data()), py::cast(points));
    
    // Verify data is shared
    auto unchecked = arr.unchecked<2>();
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(unchecked(i, 0), points[i].x);
        EXPECT_FLOAT_EQ(unchecked(i, 1), points[i].y);
    }
    
    // Modify through numpy array
    auto mutable_unchecked = arr.mutable_unchecked<2>();
    mutable_unchecked(0, 0) = 999.0f;
    
    // Verify change is reflected in original data
    EXPECT_FLOAT_EQ(points[0].x, 999.0f);
}

TEST_F(ConversionTest, BufferProtocol) {
    // Test Point2D buffer protocol
    {
        Point2D p(3.14f, 2.71f);
        py::buffer_info info = py::buffer_info(
            &p.x, sizeof(float), py::format_descriptor<float>::format(),
            1, {2}, {sizeof(float)}
        );
        
        EXPECT_EQ(info.itemsize, sizeof(float));
        EXPECT_EQ(info.ndim, 1);
        EXPECT_EQ(info.shape[0], 2);
    }
    
    // Test Transform2D buffer protocol
    {
        Transform2D t = Transform2D::identity();
        py::buffer_info info = py::buffer_info(
            t.data(), sizeof(float), py::format_descriptor<float>::format(),
            2, {3, 3}, {3 * sizeof(float), sizeof(float)}
        );
        
        EXPECT_EQ(info.itemsize, sizeof(float));
        EXPECT_EQ(info.ndim, 2);
        EXPECT_EQ(info.shape[0], 3);
        EXPECT_EQ(info.shape[1], 3);
    }
}