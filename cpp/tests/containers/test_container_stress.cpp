#include <gtest/gtest.h>
#include "claude_draw/containers/soa_container.h"
#include "claude_draw/containers/spatial_index.h"
#include "claude_draw/containers/parallel_visitor.h"
#include "claude_draw/containers/cow_container.h"
#include "claude_draw/containers/bulk_operations.h"
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <unordered_set>
#include <atomic>

using namespace claude_draw::containers;
using namespace claude_draw::shapes;
using namespace claude_draw;

class ContainerStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        rng_.seed(42);  // Consistent seed for reproducible tests
    }
    
    std::mt19937 rng_;
    std::uniform_real_distribution<float> pos_dist_{-10000.0f, 10000.0f};
    std::uniform_real_distribution<float> size_dist_{1.0f, 100.0f};
    std::uniform_int_distribution<int> type_dist_{0, 3};
};

// Stress test: Large scale insertion and removal
TEST_F(ContainerStressTest, LargeScaleInsertionRemoval) {
    SoAContainer container;
    const size_t target_size = 100000;  // Reduced for faster testing
    const size_t removal_iterations = 10;
    
    std::cout << "\nStress Test: Large Scale Insertion/Removal\n";
    
    // Phase 1: Bulk insertion
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<uint32_t> all_ids;
    all_ids.reserve(target_size);
    
    for (size_t i = 0; i < target_size; ++i) {
        uint32_t id = 0;
        switch (type_dist_(rng_)) {
            case 0:
                id = container.add_circle(Circle(pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_)));
                break;
            case 1:
                id = container.add_rectangle(Rectangle(
                    pos_dist_(rng_), pos_dist_(rng_),
                    pos_dist_(rng_), pos_dist_(rng_)));
                break;
            case 2:
                id = container.add_ellipse(Ellipse(
                    pos_dist_(rng_), pos_dist_(rng_),
                    size_dist_(rng_), size_dist_(rng_)));
                break;
            case 3:
                id = container.add_line(Line(
                    pos_dist_(rng_), pos_dist_(rng_),
                    pos_dist_(rng_), pos_dist_(rng_)));
                break;
        }
        all_ids.push_back(id);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(container.size(), target_size);
    std::cout << "  Insertion: " << insert_time.count() << " ms for " 
              << target_size << " shapes (" 
              << insert_time.count() * 1000.0 / target_size << " μs/shape)\n";
    
    // Phase 2: Random removal and re-insertion
    for (size_t iter = 0; iter < removal_iterations; ++iter) {
        // Remove 10% randomly
        std::shuffle(all_ids.begin(), all_ids.end(), rng_);
        size_t remove_count = target_size / 10;
        
        start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < remove_count; ++i) {
            container.remove(all_ids[i]);
        }
        end = std::chrono::high_resolution_clock::now();
        auto remove_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Re-insert
        start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < remove_count; ++i) {
            uint32_t new_id = container.add_circle(Circle(
                pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_)));
            all_ids[i] = new_id;
        }
        end = std::chrono::high_resolution_clock::now();
        auto reinsert_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  Iteration " << iter + 1 << ": Remove " 
                  << remove_time.count() << " ms, Re-insert " 
                  << reinsert_time.count() << " ms\n";
    }
    
    // Phase 3: Memory compaction
    start = std::chrono::high_resolution_clock::now();
    container.compact();
    end = std::chrono::high_resolution_clock::now();
    auto compact_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Compaction: " << compact_time.count() << " ms\n";
}

// Stress test: Concurrent operations
TEST_F(ContainerStressTest, ConcurrentOperations) {
    SoAContainer container;
    const size_t shapes_per_thread = 10000;  // Reduced for faster testing
    const size_t num_threads = std::thread::hardware_concurrency();
    
    std::cout << "\nStress Test: Concurrent Operations (" << num_threads << " threads)\n";
    
    // Pre-populate container
    for (size_t i = 0; i < shapes_per_thread; ++i) {
        container.add_circle(Circle(pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_)));
    }
    
    std::atomic<size_t> operations_completed(0);
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Half threads do reads, half do modifications
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&container, &operations_completed, t, num_threads]() {
            std::mt19937 local_rng(t);
            std::uniform_real_distribution<float> local_dist(-1000.0f, 1000.0f);
            
            if (t < num_threads / 2) {
                // Read operations
                for (size_t i = 0; i < 10000; ++i) {
                    size_t count = container.size();
                    BoundingBox bounds = container.get_bounds();
                    (void)count;
                    (void)bounds;
                    operations_completed++;
                }
            } else {
                // Write operations (each thread works on its own shapes)
                std::vector<uint32_t> thread_ids;
                for (size_t i = 0; i < 1000; ++i) {
                    uint32_t id = container.add_circle(Circle(
                        local_dist(local_rng) + t * 10000.0f,  // Ensure spatial separation
                        local_dist(local_rng),
                        5.0f));
                    thread_ids.push_back(id);
                    operations_completed++;
                }
                
                // Remove half
                for (size_t i = 0; i < thread_ids.size() / 2; ++i) {
                    container.remove(thread_ids[i]);
                    operations_completed++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Total operations: " << operations_completed.load() << "\n";
    std::cout << "  Duration: " << duration.count() << " ms\n";
    std::cout << "  Operations/sec: " << operations_completed.load() * 1000.0 / duration.count() << "\n";
}

// Stress test: Spatial index under heavy load
TEST_F(ContainerStressTest, SpatialIndexHeavyLoad) {
    const size_t num_shapes = 10000;  // Reduced for faster testing
    const size_t num_queries = 1000;  // Reduced for faster testing
    
    std::cout << "\nStress Test: Spatial Index Heavy Load\n";
    
    // Create spatial index
    RTree<uint32_t> index;
    std::vector<BoundingBox> all_bounds;  // Store bounds for later removal
    
    // Insert shapes
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < num_shapes; ++i) {
        float x = pos_dist_(rng_);
        float y = pos_dist_(rng_);
        float size = size_dist_(rng_);
        BoundingBox bounds(x, y, x + size, y + size);
        all_bounds.push_back(bounds);
        index.insert(bounds, static_cast<uint32_t>(i));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Insertion: " << insert_time.count() << " ms for " 
              << num_shapes << " shapes\n";
    
    // Perform random queries
    start = std::chrono::high_resolution_clock::now();
    size_t total_results = 0;
    for (size_t i = 0; i < num_queries; ++i) {
        float x = pos_dist_(rng_);
        float y = pos_dist_(rng_);
        float size = 1000.0f;  // Large query region
        BoundingBox query(x, y, x + size, y + size);
        
        std::vector<uint32_t> results;
        index.query(query, [&results](const uint32_t& id) {
            results.push_back(id);
            return true;
        });
        total_results += results.size();
    }
    end = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Queries: " << query_time.count() << " ms for " 
              << num_queries << " queries\n";
    std::cout << "  Average results per query: " << total_results / static_cast<double>(num_queries) << "\n";
    
    // Dynamic updates
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < num_shapes / 10; ++i) {
        // Remove and re-insert with new position
        uint32_t id = static_cast<uint32_t>(i);
        index.remove(all_bounds[i], id);
        
        float x = pos_dist_(rng_);
        float y = pos_dist_(rng_);
        float size = size_dist_(rng_);
        BoundingBox new_bounds(x, y, x + size, y + size);
        all_bounds[i] = new_bounds;  // Update stored bounds
        index.insert(new_bounds, id);
    }
    end = std::chrono::high_resolution_clock::now();
    auto update_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Updates: " << update_time.count() << " ms for " 
              << num_shapes / 10 << " remove/insert operations\n";
}

// Stress test: CoW container version explosion
TEST_F(ContainerStressTest, CoWVersionExplosion) {
    const size_t initial_shapes = 10000;
    const size_t num_versions = 100;
    
    std::cout << "\nStress Test: CoW Version Explosion\n";
    
    // Create version control system
    CoWVersionControl vc(num_versions);
    
    // Initial population
    auto& v0 = vc.current_mutable();
    for (size_t i = 0; i < initial_shapes; ++i) {
        v0.add_circle(Circle(pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_)));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create many versions with small changes
    for (size_t v = 0; v < num_versions - 1; ++v) {
        vc.checkpoint();
        auto& current = vc.current_mutable();
        
        // Make small changes
        for (size_t i = 0; i < 10; ++i) {
            current.add_circle(Circle(pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_)));
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto version_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Version creation: " << version_time.count() << " ms for " 
              << num_versions << " versions\n";
    
    // Random version jumping
    start = std::chrono::high_resolution_clock::now();
    std::uniform_int_distribution<size_t> version_dist(0, num_versions - 1);
    for (size_t i = 0; i < 1000; ++i) {
        size_t target_version = version_dist(rng_);
        vc.go_to_version(target_version);
        size_t size = vc.current().size();
        (void)size;
    }
    end = std::chrono::high_resolution_clock::now();
    auto jump_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Version jumping: " << jump_time.count() << " ms for 1000 jumps\n";
    
    // Memory usage test - create many CoW copies
    std::vector<CoWContainer> copies;
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 1000; ++i) {
        copies.push_back(vc.current());
    }
    end = std::chrono::high_resolution_clock::now();
    auto copy_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "  CoW copying: " << copy_time.count() << " μs for 1000 copies ("
              << copy_time.count() / 1000.0 << " μs/copy)\n";
}

// Stress test: Extreme bounds calculations
TEST_F(ContainerStressTest, ExtremeBoundsCalculations) {
    SoAContainer container;
    const size_t num_shapes = 10000;  // Reduced for faster testing
    
    std::cout << "\nStress Test: Extreme Bounds Calculations\n";
    
    // Add shapes at extreme positions
    std::uniform_real_distribution<float> extreme_dist(-1e6f, 1e6f);
    
    for (size_t i = 0; i < num_shapes; ++i) {
        float x = extreme_dist(rng_);
        float y = extreme_dist(rng_);
        
        if (i % 4 == 0) {
            container.add_circle(Circle(x, y, 1.0f));
        } else if (i % 4 == 1) {
            container.add_rectangle(Rectangle(x, y, x + 1.0f, y + 1.0f));
        } else if (i % 4 == 2) {
            container.add_ellipse(Ellipse(x, y, 1.0f, 0.5f));
        } else {
            container.add_line(Line(x, y, x + 1.0f, y + 1.0f));
        }
    }
    
    // Incremental bounds updates
    auto start = std::chrono::high_resolution_clock::now();
    BoundingBox bounds1 = container.get_bounds();
    auto end = std::chrono::high_resolution_clock::now();
    auto first_calc = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Add more shapes
    for (size_t i = 0; i < 1000; ++i) {
        container.add_circle(Circle(extreme_dist(rng_), extreme_dist(rng_), 1.0f));
    }
    
    start = std::chrono::high_resolution_clock::now();
    BoundingBox bounds2 = container.get_bounds();
    end = std::chrono::high_resolution_clock::now();
    auto incremental_calc = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "  First calculation: " << first_calc.count() << " μs\n";
    std::cout << "  Incremental update: " << incremental_calc.count() << " μs\n";
    std::cout << "  Bounds: [" << bounds2.min_x << ", " << bounds2.min_y 
              << "] to [" << bounds2.max_x << ", " << bounds2.max_y << "]\n";
    
    // Force recalculation
    container.set_visible(1, false);  // This marks bounds dirty
    
    start = std::chrono::high_resolution_clock::now();
    BoundingBox bounds3 = container.get_bounds();
    end = std::chrono::high_resolution_clock::now();
    auto recalc_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Full recalculation: " << recalc_time.count() << " ms\n";
}

// Stress test: Memory fragmentation
TEST_F(ContainerStressTest, MemoryFragmentation) {
    SoAContainer container;
    const size_t iterations = 100;
    const size_t shapes_per_iteration = 10000;
    
    std::cout << "\nStress Test: Memory Fragmentation\n";
    
    std::vector<uint32_t> all_ids;
    
    for (size_t iter = 0; iter < iterations; ++iter) {
        // Add shapes
        for (size_t i = 0; i < shapes_per_iteration; ++i) {
            uint32_t id = container.add_circle(Circle(
                pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_)));
            all_ids.push_back(id);
        }
        
        // Remove every other shape
        for (size_t i = iter * shapes_per_iteration; i < all_ids.size(); i += 2) {
            container.remove(all_ids[i]);
        }
        
        if (iter % 10 == 0) {
            std::cout << "  Iteration " << iter << ": size=" << container.size() 
                      << ", capacity utilization=" 
                      << (container.size() * 100.0 / ((iter + 1) * shapes_per_iteration)) 
                      << "%\n";
        }
    }
    
    // Final compaction
    auto start = std::chrono::high_resolution_clock::now();
    container.compact();
    auto end = std::chrono::high_resolution_clock::now();
    auto compact_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Final compaction: " << compact_time.count() << " ms\n";
    std::cout << "  Final size: " << container.size() << "\n";
}

// Stress test: Parallel visitor scalability
TEST_F(ContainerStressTest, ParallelVisitorScalability) {
    SoAContainer container;
    const size_t num_shapes = 100000;  // Reduced for faster testing
    
    std::cout << "\nStress Test: Parallel Visitor Scalability\n";
    
    // Populate container
    for (size_t i = 0; i < num_shapes; ++i) {
        container.add_circle(Circle(i * 0.1f, i * 0.2f, 1.0f));
    }
    
    // Test with different thread counts
    std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};
    
    for (size_t threads : thread_counts) {
        if (threads > std::thread::hardware_concurrency()) {
            continue;
        }
        
        ParallelVisitor::Config config;
        config.thread_count = threads;
        ParallelVisitor visitor(config);
        
        // Complex computation
        auto start = std::chrono::high_resolution_clock::now();
        
        std::atomic<double> total_area(0.0);
        visitor.visit_circles(container, [&total_area](CircleShape& circle, size_t) {
            // Simulate complex computation
            double area = M_PI * circle.radius * circle.radius;
            for (int i = 0; i < 10; ++i) {
                area = std::sqrt(area * area);
            }
            total_area.fetch_add(area, std::memory_order_relaxed);
        });
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  Threads: " << threads << ", Time: " << duration.count() << " ms\n";
    }
}