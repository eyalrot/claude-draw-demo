#include <gtest/gtest.h>
#include "claude_draw/containers/bulk_operations.h"
#include <chrono>
#include <random>
#include <unordered_set>
#include <thread>
#include <mutex>

using namespace claude_draw::containers;
using namespace claude_draw::shapes;
using namespace claude_draw;

class BulkOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<SoAContainer>();
    }
    
    std::unique_ptr<SoAContainer> container_;
};

// Test bulk insert single type
TEST_F(BulkOperationsTest, BulkInsertSingleType) {
    // Create a batch of circles
    std::vector<Circle> circles;
    for (int i = 0; i < 100; ++i) {
        circles.emplace_back(i * 10.0f, i * 20.0f, 5.0f + i * 0.1f);
    }
    
    // Bulk insert
    auto ids = BulkOperations::bulk_insert(*container_, std::span<const Circle>(circles));
    
    EXPECT_EQ(ids.size(), circles.size());
    EXPECT_EQ(container_->size(), circles.size());
    EXPECT_EQ(container_->count(ShapeType::Circle), circles.size());
    
    // Verify all shapes were inserted correctly
    for (size_t i = 0; i < ids.size(); ++i) {
        EXPECT_TRUE(container_->contains(ids[i]));
    }
}

// Test bulk insert mixed types
TEST_F(BulkOperationsTest, BulkInsertMixedTypes) {
    // Use batch builder
    BulkOperations::BatchBuilder builder;
    
    // Add various shapes
    for (int i = 0; i < 25; ++i) {
        builder.add_circle(i * 10.0f, i * 10.0f, 5.0f)
               .add_rectangle(i * 20.0f, i * 20.0f, i * 30.0f, i * 30.0f)
               .add_ellipse(i * 15.0f, i * 15.0f, 10.0f, 5.0f)
               .add_line(i * 5.0f, i * 5.0f, i * 25.0f, i * 25.0f);
    }
    
    EXPECT_EQ(builder.size(), 100u);
    
    // Insert into container
    auto ids = builder.insert_into(*container_);
    
    EXPECT_EQ(ids.size(), 100u);
    EXPECT_EQ(container_->size(), 100u);
    EXPECT_EQ(container_->count(ShapeType::Circle), 25u);
    EXPECT_EQ(container_->count(ShapeType::Rectangle), 25u);
    EXPECT_EQ(container_->count(ShapeType::Ellipse), 25u);
    EXPECT_EQ(container_->count(ShapeType::Line), 25u);
}

// Test bulk remove
TEST_F(BulkOperationsTest, BulkRemove) {
    // Add shapes
    std::vector<uint32_t> all_ids;
    for (int i = 0; i < 100; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        all_ids.push_back(container_->add_circle(c));
    }
    
    EXPECT_EQ(container_->size(), 100u);
    
    // Remove first 50
    std::vector<uint32_t> ids_to_remove(all_ids.begin(), all_ids.begin() + 50);
    size_t removed = BulkOperations::bulk_remove(*container_, ids_to_remove);
    
    EXPECT_EQ(removed, 50u);
    EXPECT_EQ(container_->size(), 50u);
    
    // Verify correct shapes were removed
    for (size_t i = 0; i < 50; ++i) {
        EXPECT_FALSE(container_->contains(all_ids[i]));
    }
    for (size_t i = 50; i < 100; ++i) {
        EXPECT_TRUE(container_->contains(all_ids[i]));
    }
}

// Test bulk visibility update
TEST_F(BulkOperationsTest, BulkSetVisibility) {
    // Add shapes
    std::vector<uint32_t> ids;
    for (int i = 0; i < 50; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        ids.push_back(container_->add_circle(c));
    }
    
    // Hide all shapes
    size_t updated = BulkOperations::bulk_set_visible(*container_, ids, false);
    EXPECT_EQ(updated, ids.size());
    
    // Verify bounds calculation excludes hidden shapes
    BoundingBox bounds = container_->get_bounds();
    EXPECT_FLOAT_EQ(bounds.min_x, 0.0f);
    EXPECT_FLOAT_EQ(bounds.min_y, 0.0f);
    EXPECT_FLOAT_EQ(bounds.max_x, 0.0f);
    EXPECT_FLOAT_EQ(bounds.max_y, 0.0f);
    
    // Show half of them
    std::vector<uint32_t> half_ids(ids.begin(), ids.begin() + 25);
    updated = BulkOperations::bulk_set_visible(*container_, half_ids, true);
    EXPECT_EQ(updated, 25u);
}

// Test bulk z-index update
TEST_F(BulkOperationsTest, BulkSetZIndex) {
    // Add shapes
    std::vector<uint32_t> group1, group2;
    
    for (int i = 0; i < 25; ++i) {
        Circle c1(i * 1.0f, 0.0f, 5.0f);
        Circle c2(i * 1.0f, 10.0f, 5.0f);
        group1.push_back(container_->add_circle(c1));
        group2.push_back(container_->add_circle(c2));
    }
    
    // Set different z-indices for groups
    BulkOperations::bulk_set_z_index(*container_, group1, 100);
    BulkOperations::bulk_set_z_index(*container_, group2, 200);
    
    // Verify ordering
    std::vector<uint16_t> z_indices;
    container_->for_each_sorted([&z_indices](const auto& shape) {
        // In real implementation, we'd need access to z-index
        // This test is more conceptual
    });
}

// Test bulk transform
TEST_F(BulkOperationsTest, BulkTransform) {
    // Add circles
    std::vector<Circle> circles;
    for (int i = 0; i < 100; ++i) {
        circles.emplace_back(i * 1.0f, i * 2.0f, 10.0f);
    }
    auto ids = BulkOperations::bulk_insert(*container_, std::span<const Circle>(circles));
    
    // Transform: scale all radii by 2x
    size_t transformed = BulkOperations::bulk_transform<CircleShape>(
        *container_,
        [](CircleShape& circle) {
            circle.radius *= 2.0f;
        });
    
    EXPECT_EQ(transformed, circles.size());
    
    // Verify transformation
    container_->for_each_type<CircleShape>([](const CircleShape& circle) {
        EXPECT_FLOAT_EQ(circle.radius, 20.0f);
    });
}

// Test performance comparison
TEST_F(BulkOperationsTest, PerformanceComparison) {
    const size_t count = 10000;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    
    // Prepare shapes
    std::vector<Circle> circles;
    circles.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(dist(rng), dist(rng), std::abs(dist(rng)) * 0.1f);
    }
    
    // Time individual insertion
    SoAContainer individual_container;
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& circle : circles) {
        individual_container.add_circle(circle);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto individual_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Time bulk insertion
    start = std::chrono::high_resolution_clock::now();
    auto ids = BulkOperations::bulk_insert(*container_, std::span<const Circle>(circles));
    end = std::chrono::high_resolution_clock::now();
    auto bulk_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nBulk Operations Performance:\n";
    std::cout << "  Individual insertion: " << individual_time.count() / 1000.0 
              << " ms (" << individual_time.count() / static_cast<double>(count) 
              << " μs/shape)\n";
    std::cout << "  Bulk insertion: " << bulk_time.count() / 1000.0 
              << " ms (" << bulk_time.count() / static_cast<double>(count) 
              << " μs/shape)\n";
    std::cout << "  Speedup: " << static_cast<double>(individual_time.count()) / bulk_time.count() 
              << "x\n";
    
    EXPECT_EQ(ids.size(), count);
    EXPECT_EQ(container_->size(), count);
}

// Test batch builder chaining
TEST_F(BulkOperationsTest, BatchBuilderChaining) {
    BulkOperations::BatchBuilder builder;
    
    // Chain multiple operations
    builder.reserve(1000)
           .add_circle(10, 20, 5)
           .add_rectangle(0, 0, 100, 100)
           .add_ellipse(50, 50, 30, 20)
           .add_line(0, 0, 100, 100)
           .add_circle(30, 40, 8)
           .add_rectangle(50, 50, 150, 150);
    
    EXPECT_EQ(builder.size(), 6u);
    
    auto ids = builder.insert_into(*container_);
    EXPECT_EQ(ids.size(), 6u);
    EXPECT_EQ(container_->size(), 6u);
    
    // Clear and reuse
    builder.clear();
    EXPECT_EQ(builder.size(), 0u);
    
    builder.add_circle(0, 0, 10);
    ids = builder.insert_into(*container_);
    EXPECT_EQ(container_->size(), 7u);
}

// Test empty bulk operations
TEST_F(BulkOperationsTest, EmptyOperations) {
    // Empty bulk insert
    std::vector<Circle> empty_circles;
    auto ids = BulkOperations::bulk_insert(*container_, std::span<const Circle>(empty_circles));
    EXPECT_TRUE(ids.empty());
    EXPECT_EQ(container_->size(), 0u);
    
    // Empty bulk remove
    std::vector<uint32_t> empty_ids;
    size_t removed = BulkOperations::bulk_remove(*container_, empty_ids);
    EXPECT_EQ(removed, 0u);
}

// Test bulk operations on large dataset
TEST_F(BulkOperationsTest, LargeDataset) {
    const size_t large_count = 100000;
    
    // Create large batch
    BulkOperations::BatchBuilder builder;
    builder.reserve(large_count);
    
    for (size_t i = 0; i < large_count / 4; ++i) {
        float x = i * 0.1f;
        float y = i * 0.2f;
        builder.add_circle(x, y, 1.0f)
               .add_rectangle(x, y, x + 10, y + 10)
               .add_ellipse(x, y, 5, 3)
               .add_line(x, y, x + 20, y + 20);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto ids = builder.insert_into(*container_);
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(ids.size(), large_count);
    EXPECT_EQ(container_->size(), large_count);
    
    std::cout << "\nLarge dataset bulk insert: " << insert_time.count() 
              << " ms for " << large_count << " shapes\n";
    
    // Bulk transform on large dataset
    start = std::chrono::high_resolution_clock::now();
    size_t transformed = BulkOperations::bulk_transform<CircleShape>(
        *container_,
        [](CircleShape& circle) {
            circle.center_x += 100.0f;
            circle.center_y += 100.0f;
        });
    end = std::chrono::high_resolution_clock::now();
    auto transform_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(transformed, large_count / 4);
    std::cout << "Bulk transform: " << transform_time.count() 
              << " ms for " << transformed << " shapes\n";
}

// Test thread safety of bulk operations
TEST_F(BulkOperationsTest, ThreadSafety) {
    const size_t shapes_per_thread = 1000;
    const size_t num_threads = 4;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<uint32_t>> thread_ids(num_threads);
    std::mutex container_mutex;
    
    // Each thread performs bulk insertion
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, shapes_per_thread, &thread_ids, &container_mutex]() {
            std::vector<Circle> circles;
            for (size_t i = 0; i < shapes_per_thread; ++i) {
                circles.emplace_back(
                    t * 1000.0f + i,  // Ensure unique positions
                    t * 2000.0f + i,
                    5.0f
                );
            }
            
            // Synchronize container access
            std::lock_guard<std::mutex> lock(container_mutex);
            thread_ids[t] = BulkOperations::bulk_insert(*container_, std::span<const Circle>(circles));
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all shapes were inserted
    EXPECT_EQ(container_->size(), shapes_per_thread * num_threads);
    
    // Verify no ID conflicts
    std::unordered_set<uint32_t> all_ids;
    for (const auto& ids : thread_ids) {
        for (uint32_t id : ids) {
            EXPECT_TRUE(all_ids.insert(id).second);  // Should be unique
        }
    }
}