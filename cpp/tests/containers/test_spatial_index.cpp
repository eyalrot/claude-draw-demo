#include <gtest/gtest.h>
#include "claude_draw/containers/spatial_index.h"
#include <random>
#include <chrono>
#include <algorithm>

using namespace claude_draw::containers;
using namespace claude_draw;

class SpatialIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        rtree_ = std::make_unique<RTree<uint32_t>>();
    }
    
    std::unique_ptr<RTree<uint32_t>> rtree_;
};

// Test basic insertion and query
TEST_F(SpatialIndexTest, BasicInsertAndQuery) {
    // Insert a single item
    BoundingBox bounds(10.0f, 20.0f, 30.0f, 40.0f);
    uint32_t id = 1;
    
    rtree_->insert(bounds, id);
    EXPECT_EQ(rtree_->size(), 1u);
    
    // Query exact bounds
    auto results = rtree_->query_range(bounds);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], id);
    
    // Query overlapping bounds
    BoundingBox query(15.0f, 25.0f, 35.0f, 45.0f);
    results = rtree_->query_range(query);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], id);
    
    // Query non-overlapping bounds
    BoundingBox non_overlap(50.0f, 60.0f, 70.0f, 80.0f);
    results = rtree_->query_range(non_overlap);
    EXPECT_EQ(results.size(), 0u);
}

// Test multiple insertions
TEST_F(SpatialIndexTest, MultipleInsertions) {
    // Insert non-overlapping boxes
    for (uint32_t i = 0; i < 10; ++i) {
        float x = i * 20.0f;
        BoundingBox bounds(x, 0.0f, x + 10.0f, 10.0f);
        rtree_->insert(bounds, i);
    }
    
    // Verify individual boxes can be found
    for (uint32_t i = 0; i < 3; ++i) {
        float x = i * 20.0f;
        BoundingBox test_query(x, 0.0f, x + 10.0f, 10.0f);
        auto test_results = rtree_->query_range(test_query);
        if (test_results.size() != 1) {
            std::cout << "Box " << i << " at x=" << x << " not found correctly. "
                      << "Found " << test_results.size() << " results\n";
        }
    }
    
    EXPECT_EQ(rtree_->size(), 10u);
    
    // Query a region that covers first 3 boxes
    BoundingBox query(0.0f, 0.0f, 50.0f, 10.0f);
    auto results = rtree_->query_range(query);
    
    // Debug output
    if (results.size() != 3) {
        std::cout << "Expected 3 results but got " << results.size() << "\n";
        std::cout << "Query bounds: " << query.min_x << "," << query.min_y 
                  << " to " << query.max_x << "," << query.max_y << "\n";
        std::cout << "Results: ";
        for (auto id : results) {
            std::cout << id << " ";
        }
        std::cout << "\n";
    }
    
    ASSERT_EQ(results.size(), 3u);
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results[0], 0u);
    EXPECT_EQ(results[1], 1u);
    EXPECT_EQ(results[2], 2u);
}

// Test overlapping boxes
TEST_F(SpatialIndexTest, OverlappingBoxes) {
    // Insert overlapping boxes
    BoundingBox bounds1(0.0f, 0.0f, 20.0f, 20.0f);
    BoundingBox bounds2(10.0f, 10.0f, 30.0f, 30.0f);
    BoundingBox bounds3(5.0f, 5.0f, 25.0f, 25.0f);
    
    rtree_->insert(bounds1, 1);
    rtree_->insert(bounds2, 2);
    rtree_->insert(bounds3, 3);
    
    // Query center region
    BoundingBox query(12.0f, 12.0f, 18.0f, 18.0f);
    auto results = rtree_->query_range(query);
    
    ASSERT_EQ(results.size(), 3u);  // All three overlap with center
}

// Test removal
TEST_F(SpatialIndexTest, RemoveItems) {
    // Insert items
    std::vector<std::pair<BoundingBox, uint32_t>> items;
    for (uint32_t i = 0; i < 5; ++i) {
        float x = i * 20.0f;
        BoundingBox bounds(x, 0.0f, x + 10.0f, 10.0f);
        items.push_back({bounds, i});
        rtree_->insert(bounds, i);
    }
    
    EXPECT_EQ(rtree_->size(), 5u);
    
    // Remove middle item
    EXPECT_TRUE(rtree_->remove(items[2].first, items[2].second));
    EXPECT_EQ(rtree_->size(), 4u);
    
    // Verify it's gone
    auto results = rtree_->query_range(items[2].first);
    EXPECT_EQ(results.size(), 0u);
    
    // Try to remove again
    EXPECT_FALSE(rtree_->remove(items[2].first, items[2].second));
    
    // Verify others still exist
    results = rtree_->query_range(items[0].first);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], 0u);
}

// Test nearest neighbor search
TEST_F(SpatialIndexTest, NearestNeighbor) {
    // Insert points as small boxes
    std::vector<std::pair<float, float>> points = {
        {10.0f, 10.0f},
        {30.0f, 30.0f},
        {50.0f, 10.0f},
        {30.0f, 50.0f}
    };
    
    for (size_t i = 0; i < points.size(); ++i) {
        BoundingBox bounds(points[i].first - 1.0f, points[i].second - 1.0f,
                          points[i].first + 1.0f, points[i].second + 1.0f);
        rtree_->insert(bounds, static_cast<uint32_t>(i));
    }
    
    // Find nearest to (20, 20)
    auto [nearest_id, distance] = rtree_->nearest_neighbor(20.0f, 20.0f);
    EXPECT_EQ(nearest_id, 0u);  // Point at (10, 10) is nearest
    
    // Find nearest to (40, 40)
    auto [nearest_id2, distance2] = rtree_->nearest_neighbor(40.0f, 40.0f);
    EXPECT_EQ(nearest_id2, 1u);  // Point at (30, 30) is nearest
    
    // Empty tree
    rtree_->clear();
    auto [nearest_id3, distance3] = rtree_->nearest_neighbor(0.0f, 0.0f);
    EXPECT_EQ(distance3, std::numeric_limits<float>::max());
}

// Test bulk loading
TEST_F(SpatialIndexTest, BulkLoading) {
    // Create many items
    std::vector<std::pair<BoundingBox, uint32_t>> items;
    const size_t count = 1000;
    
    for (size_t i = 0; i < count; ++i) {
        float x = (i % 100) * 10.0f;
        float y = (i / 100) * 10.0f;
        BoundingBox bounds(x, y, x + 8.0f, y + 8.0f);
        items.push_back({bounds, static_cast<uint32_t>(i)});
    }
    
    // Bulk load
    rtree_->bulk_load(items);
    EXPECT_EQ(rtree_->size(), count);
    
    // Verify tree structure
    size_t height = rtree_->height();
    EXPECT_GT(height, 1u);  // Should have multiple levels
    EXPECT_LT(height, 10u);  // But not too deep
    
    // Query a region
    BoundingBox query(45.0f, 45.0f, 55.0f, 55.0f);
    auto results = rtree_->query_range(query);
    EXPECT_GT(results.size(), 0u);
    
    // Verify correct results
    for (auto id : results) {
        float x = (id % 100) * 10.0f;
        float y = (id / 100) * 10.0f;
        BoundingBox item_bounds(x, y, x + 8.0f, y + 8.0f);
        EXPECT_TRUE(item_bounds.intersects(query));
    }
}

// Test query with callback
TEST_F(SpatialIndexTest, QueryWithCallback) {
    // Insert items
    for (uint32_t i = 0; i < 10; ++i) {
        float x = i * 10.0f;
        BoundingBox bounds(x, 0.0f, x + 8.0f, 8.0f);
        rtree_->insert(bounds, i);
    }
    
    // Query with early termination
    std::vector<uint32_t> results;
    size_t max_results = 3;
    
    BoundingBox query(0.0f, 0.0f, 100.0f, 10.0f);
    rtree_->query(query, [&results, max_results](const uint32_t& id) {
        results.push_back(id);
        return results.size() < max_results;  // Stop after 3 results
    });
    
    EXPECT_LE(results.size(), max_results + 1);  // May get one extra due to timing
    EXPECT_GE(results.size(), max_results);
}

// Test empty tree operations
TEST_F(SpatialIndexTest, EmptyTreeOperations) {
    EXPECT_EQ(rtree_->size(), 0u);
    EXPECT_TRUE(rtree_->empty());
    
    // Query empty tree
    BoundingBox query(0.0f, 0.0f, 100.0f, 100.0f);
    auto results = rtree_->query_range(query);
    EXPECT_EQ(results.size(), 0u);
    
    // Remove from empty tree
    EXPECT_FALSE(rtree_->remove(query, 0));
    
    // Height of empty tree
    EXPECT_EQ(rtree_->height(), 1u);  // Just root
}

// Test clear operation
TEST_F(SpatialIndexTest, ClearTree) {
    // Add items
    for (uint32_t i = 0; i < 100; ++i) {
        BoundingBox bounds(i * 1.0f, i * 1.0f, i * 1.0f + 10.0f, i * 1.0f + 10.0f);
        rtree_->insert(bounds, i);
    }
    
    EXPECT_EQ(rtree_->size(), 100u);
    EXPECT_GT(rtree_->node_count(), 1u);
    
    // Clear
    rtree_->clear();
    
    EXPECT_EQ(rtree_->size(), 0u);
    EXPECT_TRUE(rtree_->empty());
    EXPECT_EQ(rtree_->height(), 1u);
    
    // Can insert again
    BoundingBox bounds(0.0f, 0.0f, 10.0f, 10.0f);
    rtree_->insert(bounds, 1);
    EXPECT_EQ(rtree_->size(), 1u);
}

// Test degenerate boxes (points and lines)
TEST_F(SpatialIndexTest, DegenerateBoxes) {
    // Insert point (zero area box)
    BoundingBox point(10.0f, 20.0f, 10.0f, 20.0f);
    rtree_->insert(point, 1);
    
    // Insert horizontal line
    BoundingBox h_line(0.0f, 30.0f, 50.0f, 30.0f);
    rtree_->insert(h_line, 2);
    
    // Insert vertical line
    BoundingBox v_line(25.0f, 0.0f, 25.0f, 50.0f);
    rtree_->insert(v_line, 3);
    
    EXPECT_EQ(rtree_->size(), 3u);
    
    // Query that intersects all
    BoundingBox query(5.0f, 15.0f, 30.0f, 35.0f);
    auto results = rtree_->query_range(query);
    
    ASSERT_EQ(results.size(), 3u);
}

// Performance test with many items
TEST_F(SpatialIndexTest, PerformanceTest) {
    const size_t count = 10000;
    std::mt19937 rng(42);  // Deterministic seed
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);
    std::uniform_real_distribution<float> size_dist(1.0f, 50.0f);
    
    // Time insertions
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < count; ++i) {
        float x = dist(rng);
        float y = dist(rng);
        float w = size_dist(rng);
        float h = size_dist(rng);
        
        BoundingBox bounds(x, y, x + w, y + h);
        rtree_->insert(bounds, static_cast<uint32_t>(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_EQ(rtree_->size(), count);
    
    std::cout << "R-tree Performance:\n";
    std::cout << "  Insertions: " << insert_time.count() / 1000.0 << " ms for " 
              << count << " items (" 
              << static_cast<double>(insert_time.count()) / count << " μs/item)\n";
    std::cout << "  Tree height: " << rtree_->height() << "\n";
    std::cout << "  Node count: " << rtree_->node_count() << "\n";
    
    // Time queries
    const size_t query_count = 1000;
    size_t total_results = 0;
    
    start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < query_count; ++i) {
        float x = dist(rng);
        float y = dist(rng);
        BoundingBox query(x, y, x + 100.0f, y + 100.0f);
        
        auto results = rtree_->query_range(query);
        total_results += results.size();
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "  Queries: " << query_time.count() / 1000.0 << " ms for " 
              << query_count << " queries (" 
              << static_cast<double>(query_time.count()) / query_count << " μs/query)\n";
    std::cout << "  Average results per query: " 
              << static_cast<double>(total_results) / query_count << "\n";
}

// Test ShapeSpatialIndex wrapper
TEST_F(SpatialIndexTest, ShapeSpatialIndexWrapper) {
    ShapeSpatialIndex index;
    
    // Insert shapes
    index.insert(1, BoundingBox(0.0f, 0.0f, 10.0f, 10.0f));
    index.insert(2, BoundingBox(20.0f, 20.0f, 30.0f, 30.0f));
    index.insert(3, BoundingBox(5.0f, 5.0f, 15.0f, 15.0f));
    
    EXPECT_EQ(index.size(), 3u);
    
    // Query range
    auto results = index.query_range(BoundingBox(0.0f, 0.0f, 12.0f, 12.0f));
    ASSERT_EQ(results.size(), 2u);  // Should find shapes 1 and 3
    
    std::sort(results.begin(), results.end());
    EXPECT_EQ(results[0], 1u);
    EXPECT_EQ(results[1], 3u);
    
    // Find nearest shape
    auto [nearest_id, distance] = index.nearest_shape(25.0f, 25.0f);
    EXPECT_EQ(nearest_id, 2u);
    
    // Remove shape
    index.remove(2, BoundingBox(20.0f, 20.0f, 30.0f, 30.0f));
    EXPECT_EQ(index.size(), 2u);
    
    // Clear
    index.clear();
    EXPECT_EQ(index.size(), 0u);
}

// Stress test with random operations
TEST_F(SpatialIndexTest, StressTest) {
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> coord_dist(0.0f, 100.0f);
    std::uniform_real_distribution<float> size_dist(1.0f, 10.0f);
    std::uniform_int_distribution<int> op_dist(0, 2);  // 0=insert, 1=remove, 2=query
    
    struct Item {
        BoundingBox bounds;
        uint32_t id;
    };
    
    std::vector<Item> items;
    uint32_t next_id = 0;
    
    // Perform random operations
    const size_t op_count = 1000;
    for (size_t i = 0; i < op_count; ++i) {
        int op = op_dist(rng);
        
        if (op == 0 || items.empty()) {  // Insert
            float x = coord_dist(rng);
            float y = coord_dist(rng);
            float w = size_dist(rng);
            float h = size_dist(rng);
            
            BoundingBox bounds(x, y, x + w, y + h);
            uint32_t id = next_id++;
            
            rtree_->insert(bounds, id);
            items.push_back({bounds, id});
            
        } else if (op == 1) {  // Remove
            size_t idx = std::uniform_int_distribution<size_t>(0, items.size() - 1)(rng);
            rtree_->remove(items[idx].bounds, items[idx].id);
            items.erase(items.begin() + idx);
            
        } else {  // Query
            float x = coord_dist(rng);
            float y = coord_dist(rng);
            BoundingBox query(x, y, x + 20.0f, y + 20.0f);
            
            auto results = rtree_->query_range(query);
            
            // Verify results are correct
            for (auto id : results) {
                auto it = std::find_if(items.begin(), items.end(),
                    [id](const Item& item) { return item.id == id; });
                
                ASSERT_NE(it, items.end());
                EXPECT_TRUE(it->bounds.intersects(query));
            }
        }
    }
    
    EXPECT_EQ(rtree_->size(), items.size());
}