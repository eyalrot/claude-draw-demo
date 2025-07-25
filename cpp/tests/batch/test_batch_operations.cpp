#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <numeric>

#include "claude_draw/batch/batch_operations.h"

using namespace claude_draw;
using namespace claude_draw::batch;

class BatchOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Seed random number generator for reproducible tests
        rng_.seed(42);
    }
    
    std::mt19937 rng_;
};

TEST_F(BatchOperationsTest, CreatePointsBatch) {
    const size_t count = 1000;
    
    // Test uniform creation
    auto points = BatchCreator::create_points(count, 1.5f, 2.5f);
    EXPECT_EQ(points.size(), count);
    
    for (const auto* p : points) {
        EXPECT_FLOAT_EQ(p->x, 1.5f);
        EXPECT_FLOAT_EQ(p->y, 2.5f);
    }
    
    // Clean up
    BatchCreator::destroy_batch(points);
}

TEST_F(BatchOperationsTest, CreatePointsFromArrays) {
    const size_t count = 100;
    std::vector<float> x_coords(count);
    std::vector<float> y_coords(count);
    
    // Generate test data
    for (size_t i = 0; i < count; ++i) {
        x_coords[i] = static_cast<float>(i);
        y_coords[i] = static_cast<float>(i * 2);
    }
    
    auto points = BatchCreator::create_points(x_coords.data(), y_coords.data(), count);
    EXPECT_EQ(points.size(), count);
    
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(points[i]->x, static_cast<float>(i));
        EXPECT_FLOAT_EQ(points[i]->y, static_cast<float>(i * 2));
    }
    
    BatchCreator::destroy_batch(points);
}

TEST_F(BatchOperationsTest, CreatePooledPoints) {
    const size_t count = 500;
    
    auto pooled_points = BatchCreator::create_pooled_points(count, 3.14f, 2.71f);
    EXPECT_EQ(pooled_points.size(), count);
    
    for (const auto& p : pooled_points) {
        EXPECT_FLOAT_EQ(p->x, 3.14f);
        EXPECT_FLOAT_EQ(p->y, 2.71f);
    }
    
    // Pooled points are automatically cleaned up
}

TEST_F(BatchOperationsTest, CreateColorsBatch) {
    const size_t count = 1000;
    uint32_t test_color = 0xFF8040FF;  // RGBA
    
    auto colors = BatchCreator::create_colors(count, test_color);
    EXPECT_EQ(colors.size(), count);
    
    for (const auto* c : colors) {
        EXPECT_EQ(c->rgba, test_color);
    }
    
    BatchCreator::destroy_batch(colors);
}

TEST_F(BatchOperationsTest, CreateColorsFromComponents) {
    const size_t count = 100;
    std::vector<uint8_t> r(count), g(count), b(count), a(count);
    
    for (size_t i = 0; i < count; ++i) {
        r[i] = static_cast<uint8_t>(i % 256);
        g[i] = static_cast<uint8_t>((i * 2) % 256);
        b[i] = static_cast<uint8_t>((i * 3) % 256);
        a[i] = 255;
    }
    
    auto colors = BatchCreator::create_colors(r.data(), g.data(), b.data(), a.data(), count);
    EXPECT_EQ(colors.size(), count);
    
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(colors[i]->r, r[i]);
        EXPECT_EQ(colors[i]->g, g[i]);
        EXPECT_EQ(colors[i]->b, b[i]);
        EXPECT_EQ(colors[i]->a, a[i]);
    }
    
    BatchCreator::destroy_batch(colors);
}

TEST_F(BatchOperationsTest, CreateTransformsBatch) {
    const size_t count = 100;
    
    auto transforms = BatchCreator::create_transforms(count);
    EXPECT_EQ(transforms.size(), count);
    
    // All should be identity
    for (const auto* t : transforms) {
        EXPECT_FLOAT_EQ((*t)(0, 0), 1.0f);
        EXPECT_FLOAT_EQ((*t)(1, 1), 1.0f);
        EXPECT_FLOAT_EQ((*t)(2, 2), 1.0f);
        EXPECT_FLOAT_EQ((*t)(0, 1), 0.0f);
        EXPECT_FLOAT_EQ((*t)(1, 0), 0.0f);
    }
    
    BatchCreator::destroy_batch(transforms);
}

TEST_F(BatchOperationsTest, CreateBoundingBoxesBatch) {
    const size_t count = 100;
    
    auto boxes = BatchCreator::create_bounding_boxes(count, -1.0f, -2.0f, 3.0f, 4.0f);
    EXPECT_EQ(boxes.size(), count);
    
    for (const auto* box : boxes) {
        EXPECT_FLOAT_EQ(box->min_x, -1.0f);
        EXPECT_FLOAT_EQ(box->min_y, -2.0f);
        EXPECT_FLOAT_EQ(box->max_x, 3.0f);
        EXPECT_FLOAT_EQ(box->max_y, 4.0f);
    }
    
    BatchCreator::destroy_batch(boxes);
}

TEST_F(BatchOperationsTest, TransformPointsBatch) {
    const size_t count = 1000;
    
    // Create points in a contiguous batch
    ContiguousBatch<Point2D> points(count);
    for (size_t i = 0; i < count; ++i) {
        points.emplace(static_cast<float>(i), static_cast<float>(i));
    }
    
    // Create transform
    Transform2D transform = Transform2D::translate(10.0f, 20.0f);
    
    // Transform points
    BatchProcessor::transform_points(transform, points.as_span());
    
    // Verify
    auto span = points.as_span();
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(span[i].x, static_cast<float>(i) + 10.0f);
        EXPECT_FLOAT_EQ(span[i].y, static_cast<float>(i) + 20.0f);
    }
}

TEST_F(BatchOperationsTest, BlendColorsBatch) {
    const size_t count = 100;
    
    // Create test colors
    std::vector<Color> fg_colors(count);
    std::vector<Color> bg_colors(count);
    std::vector<Color> results(count);
    
    // Semi-transparent red over blue
    for (size_t i = 0; i < count; ++i) {
        fg_colors[i] = Color(255, 0, 0, 128);  // Semi-transparent red
        bg_colors[i] = Color(0, 0, 255, 255);  // Opaque blue
    }
    
    // Blend
    BatchProcessor::blend_colors(fg_colors.data(), bg_colors.data(), results.data(), count);
    
    // Verify blending results
    for (size_t i = 0; i < count; ++i) {
        // Should be purple-ish (mix of red and blue)
        EXPECT_GT(results[i].r, 0);
        EXPECT_EQ(results[i].g, 0);
        EXPECT_GT(results[i].b, 0);
        EXPECT_EQ(results[i].a, 255);
    }
}

TEST_F(BatchOperationsTest, ContainsPointsBatch) {
    const size_t count = 1000;
    
    // Create bounding box
    BoundingBox box(0.0f, 0.0f, 10.0f, 10.0f);
    
    // Create test points
    std::vector<Point2D> points(count);
    std::vector<bool> results(count);
    
    // Half inside, half outside
    for (size_t i = 0; i < count; ++i) {
        if (i < count / 2) {
            points[i] = Point2D(5.0f, 5.0f);  // Inside
        } else {
            points[i] = Point2D(15.0f, 15.0f);  // Outside
        }
    }
    
    // Test containment
    BatchProcessor::contains_points(box, points.data(), results.data(), count);
    
    // Verify
    for (size_t i = 0; i < count; ++i) {
        if (i < count / 2) {
            EXPECT_TRUE(results[i]) << "Point " << i << " should be inside";
        } else {
            EXPECT_FALSE(results[i]) << "Point " << i << " should be outside";
        }
    }
}

TEST_F(BatchOperationsTest, CalculateDistancesBatch) {
    const size_t count = 100;
    
    std::vector<Point2D> points1(count);
    std::vector<Point2D> points2(count);
    std::vector<float> distances(count);
    
    // Create test data
    for (size_t i = 0; i < count; ++i) {
        points1[i] = Point2D(0.0f, 0.0f);
        points2[i] = Point2D(3.0f, 4.0f);  // Distance should be 5.0
    }
    
    // Calculate distances
    BatchProcessor::calculate_distances(points1.data(), points2.data(), distances.data(), count);
    
    // Verify
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(distances[i], 5.0f);
    }
}

TEST_F(BatchOperationsTest, ContiguousBatchOperations) {
    ContiguousBatch<Point2D> batch(100);
    
    // Test empty batch
    EXPECT_EQ(batch.size(), 0);
    
    // Add points
    for (int i = 0; i < 50; ++i) {
        auto* p = batch.emplace(static_cast<float>(i), static_cast<float>(i * 2));
        EXPECT_EQ(p->x, static_cast<float>(i));
        EXPECT_EQ(p->y, static_cast<float>(i * 2));
    }
    
    EXPECT_EQ(batch.size(), 50);
    
    // Test span access
    auto span = batch.as_span();
    EXPECT_EQ(span.size(), 50);
    
    // Verify contiguous memory
    for (size_t i = 1; i < span.size(); ++i) {
        ptrdiff_t diff = reinterpret_cast<char*>(&span[i]) - reinterpret_cast<char*>(&span[i-1]);
        EXPECT_EQ(diff, sizeof(Point2D));
    }
    
    // Test clear
    batch.clear();
    EXPECT_EQ(batch.size(), 0);
}

TEST_F(BatchOperationsTest, PerformanceBenchmark) {
    const size_t count = 100000;
    
    // Benchmark batch creation vs individual
    auto start = std::chrono::high_resolution_clock::now();
    auto batch_points = BatchCreator::create_points(count, 1.0f, 2.0f);
    auto batch_time = std::chrono::high_resolution_clock::now() - start;
    
    start = std::chrono::high_resolution_clock::now();
    std::vector<Point2D*> individual_points;
    individual_points.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        individual_points.push_back(new Point2D(1.0f, 2.0f));
    }
    auto individual_time = std::chrono::high_resolution_clock::now() - start;
    
    double batch_ms = std::chrono::duration<double, std::milli>(batch_time).count();
    double individual_ms = std::chrono::duration<double, std::milli>(individual_time).count();
    
    std::cout << "Batch creation: " << batch_ms << " ms" << std::endl;
    std::cout << "Individual creation: " << individual_ms << " ms" << std::endl;
    std::cout << "Speedup: " << individual_ms / batch_ms << "x" << std::endl;
    
    // Clean up
    BatchCreator::destroy_batch(batch_points);
    for (auto* p : individual_points) {
        delete p;
    }
    
    // Batch should be at least as fast
    EXPECT_LE(batch_ms, individual_ms * 1.1);  // Allow 10% margin
}

TEST_F(BatchOperationsTest, ContiguousBatchPerformance) {
    const size_t count = 100000;
    
    // Benchmark contiguous batch vs vector of pointers
    auto start = std::chrono::high_resolution_clock::now();
    ContiguousBatch<Point2D> contiguous(count);
    for (size_t i = 0; i < count; ++i) {
        contiguous.emplace(static_cast<float>(i), static_cast<float>(i));
    }
    auto contiguous_time = std::chrono::high_resolution_clock::now() - start;
    
    start = std::chrono::high_resolution_clock::now();
    std::vector<Point2D*> pointers;
    pointers.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        pointers.push_back(new Point2D(static_cast<float>(i), static_cast<float>(i)));
    }
    auto pointer_time = std::chrono::high_resolution_clock::now() - start;
    
    double contiguous_ms = std::chrono::duration<double, std::milli>(contiguous_time).count();
    double pointer_ms = std::chrono::duration<double, std::milli>(pointer_time).count();
    
    std::cout << "Contiguous batch: " << contiguous_ms << " ms" << std::endl;
    std::cout << "Pointer vector: " << pointer_ms << " ms" << std::endl;
    std::cout << "Speedup: " << pointer_ms / contiguous_ms << "x" << std::endl;
    
    // Clean up pointers
    for (auto* p : pointers) {
        delete p;
    }
    
    // Contiguous should be significantly faster
    EXPECT_LT(contiguous_ms, pointer_ms);
}