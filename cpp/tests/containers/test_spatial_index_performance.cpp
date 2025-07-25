#include <gtest/gtest.h>
#include "claude_draw/containers/spatial_index.h"
#include "claude_draw/containers/soa_container.h"
#include "claude_draw/shapes/circle_optimized.h"
#include "claude_draw/shapes/rectangle_optimized.h"
#include <chrono>
#include <random>
#include <algorithm>
#include <unordered_set>

using namespace claude_draw;
using namespace claude_draw::containers;
using namespace claude_draw::shapes;

class SpatialIndexPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        rng_.seed(42);
    }
    
    std::mt19937 rng_;
    std::uniform_real_distribution<float> coord_dist_{-1000.0f, 1000.0f};
    std::uniform_real_distribution<float> size_dist_{5.0f, 50.0f};
    
    BoundingBox random_bounds() {
        float x = coord_dist_(rng_);
        float y = coord_dist_(rng_);
        float w = size_dist_(rng_);
        float h = size_dist_(rng_);
        return BoundingBox(x, y, x + w, y + h);
    }
};

// Test 1: Basic R-tree insertion and query
TEST_F(SpatialIndexPerformanceTest, BasicRTreeOperations) {
    RTree<uint32_t> rtree;
    
    // Insert shapes
    std::vector<std::pair<uint32_t, BoundingBox>> shapes;
    for (uint32_t i = 0; i < 1000; ++i) {
        BoundingBox bounds = random_bounds();
        shapes.push_back({i, bounds});
        rtree.insert(bounds, i);
    }
    
    EXPECT_EQ(rtree.size(), 1000u);
    EXPECT_GT(rtree.height(), 1u);
    EXPECT_GT(rtree.node_count(), 1u);
    
    // Query a region
    BoundingBox query_region(-100, -100, 100, 100);
    auto results = rtree.query_range(query_region);
    
    // Verify results are correct
    std::unordered_set<uint32_t> result_set(results.begin(), results.end());
    
    for (const auto& [id, bounds] : shapes) {
        bool should_be_found = bounds.intersects(query_region);
        bool was_found = result_set.count(id) > 0;
        EXPECT_EQ(should_be_found, was_found) 
            << "Shape " << id << " incorrectly " 
            << (was_found ? "included" : "excluded");
    }
}

// Test 2: Bulk loading performance
TEST_F(SpatialIndexPerformanceTest, BulkLoadingPerformance) {
    const size_t num_shapes = 100000;
    
    // Generate test data
    std::vector<std::pair<BoundingBox, uint32_t>> items;
    items.reserve(num_shapes);
    
    for (uint32_t i = 0; i < num_shapes; ++i) {
        items.push_back({random_bounds(), i});
    }
    
    // Time bulk loading
    RTree<uint32_t> rtree;
    
    auto start = std::chrono::high_resolution_clock::now();
    rtree.bulk_load(items);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double us_per_shape = static_cast<double>(duration.count()) / num_shapes;
    
    EXPECT_EQ(rtree.size(), num_shapes);
    EXPECT_LT(us_per_shape, 1.0) << "Bulk load too slow: " << us_per_shape << " μs/shape";
    
    // Log performance metrics
    std::cout << "Bulk load performance:" << std::endl;
    std::cout << "  Shapes: " << num_shapes << std::endl;
    std::cout << "  Time: " << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Per shape: " << us_per_shape << " μs" << std::endl;
    std::cout << "  Tree height: " << rtree.height() << std::endl;
    std::cout << "  Node count: " << rtree.node_count() << std::endl;
}

// Test 3: Range query performance
TEST_F(SpatialIndexPerformanceTest, RangeQueryPerformance) {
    RTree<uint32_t> rtree;
    const size_t num_shapes = 100000;
    
    // Bulk load shapes
    std::vector<std::pair<BoundingBox, uint32_t>> items;
    for (uint32_t i = 0; i < num_shapes; ++i) {
        items.push_back({random_bounds(), i});
    }
    rtree.bulk_load(items);
    
    // Perform multiple range queries
    const size_t num_queries = 1000;
    std::vector<double> query_times;
    std::vector<size_t> result_counts;
    
    for (size_t i = 0; i < num_queries; ++i) {
        BoundingBox query_bounds = random_bounds();
        
        auto start = std::chrono::high_resolution_clock::now();
        auto results = rtree.query_range(query_bounds);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        query_times.push_back(duration.count() / 1000.0);  // Convert to μs
        result_counts.push_back(results.size());
    }
    
    // Calculate statistics
    double avg_time = std::accumulate(query_times.begin(), query_times.end(), 0.0) / num_queries;
    double avg_results = std::accumulate(result_counts.begin(), result_counts.end(), 0.0) / num_queries;
    
    std::cout << "Range query performance:" << std::endl;
    std::cout << "  Avg query time: " << avg_time << " μs" << std::endl;
    std::cout << "  Avg results: " << avg_results << " shapes" << std::endl;
    
    EXPECT_LT(avg_time, 100.0) << "Queries too slow";
}

// Test 4: Nearest neighbor search
TEST_F(SpatialIndexPerformanceTest, NearestNeighborSearch) {
    RTree<uint32_t> rtree;
    
    // Insert shapes in a grid pattern for predictable results
    std::vector<std::pair<BoundingBox, uint32_t>> shapes;
    uint32_t id = 0;
    
    for (int x = -500; x <= 500; x += 100) {
        for (int y = -500; y <= 500; y += 100) {
            BoundingBox bounds(x - 10, y - 10, x + 10, y + 10);
            rtree.insert(bounds, id);
            shapes.push_back({bounds, id});
            id++;
        }
    }
    
    // Test nearest neighbor at various points
    struct TestPoint {
        float x, y;
        uint32_t expected_id;
    };
    
    std::vector<TestPoint> test_points = {
        {0, 0, 60},      // Center of grid
        {-495, -495, 0}, // Near corner
        {505, 505, 120}, // Outside grid
    };
    
    for (const auto& test : test_points) {
        auto [found_id, distance] = rtree.nearest_neighbor(test.x, test.y);
        
        // Verify it found a reasonable neighbor
        EXPECT_LT(distance, 100.0f) << "Distance too large for point (" 
                                    << test.x << ", " << test.y << ")";
        
        // For grid pattern, verify it's close to expected
        const auto& found_bounds = shapes[found_id].first;
        float center_x = (found_bounds.min_x + found_bounds.max_x) * 0.5f;
        float center_y = (found_bounds.min_y + found_bounds.max_y) * 0.5f;
        
        float dx = test.x - center_x;
        float dy = test.y - center_y;
        float actual_dist = std::sqrt(dx * dx + dy * dy);
        
        EXPECT_NEAR(distance, actual_dist, 0.001f);
    }
}

// Test 5: Dynamic insertion and removal
TEST_F(SpatialIndexPerformanceTest, DynamicOperations) {
    RTree<uint32_t> rtree;
    std::vector<std::pair<uint32_t, BoundingBox>> active_shapes;
    
    // Phase 1: Insert shapes
    for (uint32_t i = 0; i < 10000; ++i) {
        BoundingBox bounds = random_bounds();
        rtree.insert(bounds, i);
        active_shapes.push_back({i, bounds});
    }
    
    EXPECT_EQ(rtree.size(), 10000u);
    
    // Phase 2: Remove half the shapes
    std::shuffle(active_shapes.begin(), active_shapes.end(), rng_);
    size_t remove_count = active_shapes.size() / 2;
    
    for (size_t i = 0; i < remove_count; ++i) {
        bool removed = rtree.remove(active_shapes[i].second, active_shapes[i].first);
        EXPECT_TRUE(removed);
    }
    
    EXPECT_EQ(rtree.size(), 10000u - remove_count);
    
    // Phase 3: Reinsert different shapes
    for (uint32_t i = 10000; i < 15000; ++i) {
        BoundingBox bounds = random_bounds();
        rtree.insert(bounds, i);
    }
    
    EXPECT_EQ(rtree.size(), 10000u);
    
    // Verify tree is still balanced
    size_t height = rtree.height();
    size_t expected_max_height = static_cast<size_t>(
        std::ceil(std::log(10000) / std::log(4)) + 1  // Roughly log base (MAX_ENTRIES/2)
    );
    
    EXPECT_LE(height, expected_max_height * 2) << "Tree too tall, may be unbalanced";
}

// Test 6: Integration with SoA container
TEST_F(SpatialIndexPerformanceTest, SoAContainerIntegration) {
    SoAContainer container;
    ShapeSpatialIndex spatial_index;
    
    // Add shapes to both container and spatial index
    std::vector<uint32_t> shape_ids;
    
    for (int i = 0; i < 10000; ++i) {
        Circle c(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_));
        uint32_t id = container.add_circle(c);
        shape_ids.push_back(id);
        
        BoundingBox bounds = c.get_bounds();
        spatial_index.insert(id, bounds);
    }
    
    // Query a region
    BoundingBox query_region(-200, -200, 200, 200);
    auto spatial_results = spatial_index.query_range(query_region);
    
    // Verify all results exist in container
    for (uint32_t id : spatial_results) {
        EXPECT_TRUE(container.contains(id));
        
        // Get shape and verify it intersects query region
        auto* circle = container.get<CircleShape>(id);
        ASSERT_NE(circle, nullptr);
        
        Circle c(*circle);
        BoundingBox bounds = c.get_bounds();
        EXPECT_TRUE(bounds.intersects(query_region));
    }
    
    // Test removal synchronization
    for (size_t i = 0; i < 100; ++i) {
        uint32_t id = shape_ids[i];
        
        // Get bounds before removal
        auto* circle = container.get<CircleShape>(id);
        ASSERT_NE(circle, nullptr);
        Circle c(*circle);
        BoundingBox bounds = c.get_bounds();
        
        // Remove from both
        EXPECT_TRUE(container.remove(id));
        spatial_index.remove(id, bounds);
    }
    
    EXPECT_EQ(container.size(), 9900u);
    EXPECT_EQ(spatial_index.size(), 9900u);
}

// Test 7: Viewport culling performance
TEST_F(SpatialIndexPerformanceTest, ViewportCullingPerformance) {
    RTree<uint32_t> rtree;
    const size_t num_shapes = 1000000;  // 1 million shapes
    
    // Create dense scene
    std::vector<std::pair<BoundingBox, uint32_t>> items;
    for (uint32_t i = 0; i < num_shapes; ++i) {
        // Distribute shapes across large area
        float x = (i % 1000) * 50.0f - 25000.0f;
        float y = (i / 1000) * 50.0f - 25000.0f;
        BoundingBox bounds(x, y, x + 40.0f, y + 40.0f);
        items.push_back({bounds, i});
    }
    
    rtree.bulk_load(items);
    
    // Simulate viewport queries (typical screen size)
    BoundingBox viewport(0, 0, 1920, 1080);
    
    // Time multiple viewport positions
    const size_t num_viewports = 100;
    std::vector<double> cull_times;
    
    for (size_t i = 0; i < num_viewports; ++i) {
        // Move viewport
        float vx = coord_dist_(rng_) * 10;
        float vy = coord_dist_(rng_) * 10;
        BoundingBox moved_viewport(vx, vy, vx + 1920, vy + 1080);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto visible = rtree.query_range(moved_viewport);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        cull_times.push_back(duration.count() / 1000.0);  // Convert to ms
    }
    
    double avg_cull_time = std::accumulate(cull_times.begin(), cull_times.end(), 0.0) / num_viewports;
    
    std::cout << "Viewport culling performance:" << std::endl;
    std::cout << "  Scene size: " << num_shapes << " shapes" << std::endl;
    std::cout << "  Avg cull time: " << avg_cull_time << " ms" << std::endl;
    
    // Should be able to cull 1M shapes in under 1ms
    EXPECT_LT(avg_cull_time, 1.0) << "Viewport culling too slow";
}

// Test 8: Memory efficiency
TEST_F(SpatialIndexPerformanceTest, MemoryEfficiency) {
    RTree<uint32_t> rtree;
    const size_t num_shapes = 100000;
    
    // Measure memory before
    size_t initial_nodes = rtree.node_count();
    
    // Insert shapes
    for (uint32_t i = 0; i < num_shapes; ++i) {
        rtree.insert(random_bounds(), i);
    }
    
    size_t final_nodes = rtree.node_count();
    size_t nodes_created = final_nodes - initial_nodes;
    
    // Calculate memory usage (approximate)
    size_t node_size = sizeof(void*) * 10;  // Rough estimate
    size_t total_memory = nodes_created * node_size;
    double bytes_per_shape = static_cast<double>(total_memory) / num_shapes;
    
    std::cout << "Memory efficiency:" << std::endl;
    std::cout << "  Shapes: " << num_shapes << std::endl;
    std::cout << "  Nodes: " << nodes_created << std::endl;
    std::cout << "  Bytes/shape: " << bytes_per_shape << std::endl;
    std::cout << "  Tree height: " << rtree.height() << std::endl;
    
    // Should use less than 100 bytes per shape for index overhead
    EXPECT_LT(bytes_per_shape, 100.0);
}

// Test 9: Hit testing performance
TEST_F(SpatialIndexPerformanceTest, HitTestingPerformance) {
    ShapeSpatialIndex spatial_index;
    SoAContainer container;
    
    // Create scene with overlapping shapes
    for (int i = 0; i < 10000; ++i) {
        float x = coord_dist_(rng_);
        float y = coord_dist_(rng_);
        
        if (i % 3 == 0) {
            Circle c(x, y, size_dist_(rng_));
            uint32_t id = container.add_circle(c);
            spatial_index.insert(id, c.get_bounds());
        } else if (i % 3 == 1) {
            Rectangle r(x, y, size_dist_(rng_), size_dist_(rng_));
            uint32_t id = container.add_rectangle(r);
            spatial_index.insert(id, r.get_bounds());
        } else {
            Ellipse e(x, y, size_dist_(rng_), size_dist_(rng_) * 0.7f);
            uint32_t id = container.add_ellipse(e);
            spatial_index.insert(id, e.get_bounds());
        }
    }
    
    // Perform hit tests
    const size_t num_tests = 1000;
    std::vector<double> hit_times;
    size_t total_hits = 0;
    
    for (size_t i = 0; i < num_tests; ++i) {
        float test_x = coord_dist_(rng_);
        float test_y = coord_dist_(rng_);
        
        // Query small region around point
        BoundingBox point_region(test_x - 1, test_y - 1, test_x + 1, test_y + 1);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto candidates = spatial_index.query_range(point_region);
        
        // Precise hit test on candidates
        uint32_t hit_shape = 0;
        bool found = false;
        
        for (uint32_t id : candidates) {
            // This would do precise shape containment test
            // For now, just count as potential hit
            if (!found) {
                hit_shape = id;
                found = true;
                total_hits++;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        hit_times.push_back(duration.count() / 1000.0);  // Convert to μs
    }
    
    double avg_hit_time = std::accumulate(hit_times.begin(), hit_times.end(), 0.0) / num_tests;
    double hit_rate = static_cast<double>(total_hits) / num_tests * 100.0;
    
    std::cout << "Hit testing performance:" << std::endl;
    std::cout << "  Avg hit test time: " << avg_hit_time << " μs" << std::endl;
    std::cout << "  Hit rate: " << hit_rate << "%" << std::endl;
    
    EXPECT_LT(avg_hit_time, 10.0) << "Hit testing too slow";
}

// Test 10: Stress test with updates
TEST_F(SpatialIndexPerformanceTest, StressTestWithUpdates) {
    RTree<uint32_t> rtree;
    std::unordered_map<uint32_t, BoundingBox> shape_bounds;
    
    const size_t initial_shapes = 50000;
    const size_t num_operations = 100000;
    
    // Initial population
    for (uint32_t i = 0; i < initial_shapes; ++i) {
        BoundingBox bounds = random_bounds();
        rtree.insert(bounds, i);
        shape_bounds[i] = bounds;
    }
    
    // Random operations
    uint32_t next_id = initial_shapes;
    size_t inserts = 0, removes = 0, queries = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t op = 0; op < num_operations; ++op) {
        int op_type = rng_() % 10;  // 10% remove, 30% insert, 60% query
        
        if (op_type < 1 && !shape_bounds.empty()) {
            // Remove random shape
            auto it = shape_bounds.begin();
            std::advance(it, rng_() % shape_bounds.size());
            
            rtree.remove(it->second, it->first);
            shape_bounds.erase(it);
            removes++;
            
        } else if (op_type < 4) {
            // Insert new shape
            BoundingBox bounds = random_bounds();
            rtree.insert(bounds, next_id);
            shape_bounds[next_id] = bounds;
            next_id++;
            inserts++;
            
        } else {
            // Query random region
            BoundingBox query = random_bounds();
            auto results = rtree.query_range(query);
            queries++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Stress test results:" << std::endl;
    std::cout << "  Operations: " << num_operations << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    std::cout << "  Ops/sec: " << (num_operations * 1000.0 / duration.count()) << std::endl;
    std::cout << "  Inserts: " << inserts << std::endl;
    std::cout << "  Removes: " << removes << std::endl;
    std::cout << "  Queries: " << queries << std::endl;
    std::cout << "  Final size: " << rtree.size() << std::endl;
    
    EXPECT_EQ(rtree.size(), shape_bounds.size());
}