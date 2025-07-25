#include <gtest/gtest.h>
#include "claude_draw/containers/parallel_visitor.h"
#include "claude_draw/containers/soa_container.h"
#include <atomic>
#include <mutex>
#include <chrono>
#include <random>
#include <unordered_set>

using namespace claude_draw::containers;
using namespace claude_draw::shapes;
using namespace claude_draw;

class ParallelVisitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<SoAContainer>();
        visitor_ = std::make_unique<ParallelVisitor>();
    }
    
    std::unique_ptr<SoAContainer> container_;
    std::unique_ptr<ParallelVisitor> visitor_;
};

// Test basic parallel visitation
TEST_F(ParallelVisitorTest, BasicParallelVisit) {
    // Add many circles
    const size_t count = 10000;
    for (size_t i = 0; i < count; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        container_->add_circle(c);
    }
    
    // Count visits using atomic
    std::atomic<size_t> visit_count(0);
    
    visitor_->visit_circles(*container_, 
        [&visit_count](CircleShape& circle, size_t) {
            visit_count.fetch_add(1);
            // Verify data is accessible
            EXPECT_GE(circle.center_x, 0.0f);
            EXPECT_GE(circle.center_y, 0.0f);
            EXPECT_FLOAT_EQ(circle.radius, 3.0f);
        });
    
    EXPECT_EQ(visit_count.load(), count);
}

// Test parallel visit with index
TEST_F(ParallelVisitorTest, ParallelVisitWithIndex) {
    // Add shapes
    const size_t count = 1000;
    for (size_t i = 0; i < count; ++i) {
        Rectangle r(i * 10.0f, 0.0f, 10.0f, 20.0f);
        container_->add_rectangle(r);
    }
    
    // Track which indices were visited
    std::mutex visited_mutex;
    std::unordered_set<size_t> visited_indices;
    
    visitor_->visit_rectangles(*container_,
        [&](RectangleShape& rect, size_t index) {
            std::lock_guard<std::mutex> lock(visited_mutex);
            visited_indices.insert(index);
            
            // Verify expected data
            EXPECT_FLOAT_EQ(rect.x1, index * 10.0f);
            EXPECT_FLOAT_EQ(rect.y1, 0.0f);
        });
    
    // Verify all indices were visited
    EXPECT_EQ(visited_indices.size(), count);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_TRUE(visited_indices.count(i) > 0);
    }
}

// Test parallel reduction
TEST_F(ParallelVisitorTest, ParallelReduction) {
    // Add circles with known sum
    const size_t count = 10000;
    float expected_sum = 0.0f;
    
    for (size_t i = 0; i < count; ++i) {
        float radius = (i % 10) + 1.0f;
        Circle c(0.0f, 0.0f, radius);
        container_->add_circle(c);
        expected_sum += radius;
    }
    
    // Reduce to sum of radii
    float sum = visitor_->reduce_circles(
        *container_,
        0.0f,  // identity
        [](const CircleShape& c) { return c.radius; },  // reduce function
        [](float a, float b) { return a + b; }  // combine function
    );
    
    EXPECT_FLOAT_EQ(sum, expected_sum);
}

// Test parallel bounds calculation
TEST_F(ParallelVisitorTest, ParallelBoundsCalculation) {
    // Add shapes across the space
    for (int i = -50; i <= 50; i += 10) {
        for (int j = -50; j <= 50; j += 10) {
            Circle c(i * 1.0f, j * 1.0f, 5.0f);
            Rectangle r(i * 1.0f, j * 1.0f, 10.0f, 10.0f);
            container_->add_circle(c);
            container_->add_rectangle(r);
        }
    }
    
    // Calculate bounds in parallel
    auto start = std::chrono::high_resolution_clock::now();
    BoundingBox parallel_bounds = visitor_->calculate_bounds_parallel(*container_);
    auto end = std::chrono::high_resolution_clock::now();
    auto parallel_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate bounds sequentially for comparison
    start = std::chrono::high_resolution_clock::now();
    BoundingBox sequential_bounds = container_->get_bounds();
    end = std::chrono::high_resolution_clock::now();
    auto sequential_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Results should match
    EXPECT_FLOAT_EQ(parallel_bounds.min_x, sequential_bounds.min_x);
    EXPECT_FLOAT_EQ(parallel_bounds.min_y, sequential_bounds.min_y);
    EXPECT_FLOAT_EQ(parallel_bounds.max_x, sequential_bounds.max_x);
    EXPECT_FLOAT_EQ(parallel_bounds.max_y, sequential_bounds.max_y);
    
    std::cout << "Bounds calculation - Parallel: " << parallel_time.count() << " μs, "
              << "Sequential: " << sequential_time.count() << " μs\n";
}

// Test parallel transform
TEST_F(ParallelVisitorTest, ParallelTransform) {
    // Add ellipses
    const size_t count = 5000;
    for (size_t i = 0; i < count; ++i) {
        Ellipse e(i * 1.0f, i * 2.0f, 10.0f, 5.0f);
        container_->add_ellipse(e);
    }
    
    // Transform: scale all ellipses by 2x
    visitor_->visit_ellipses(*container_,
        [](EllipseShape& ellipse, size_t) {
            ellipse.radius_x *= 2.0f;
            ellipse.radius_y *= 2.0f;
        });
    
    // Verify transformation
    container_->for_each_type<EllipseShape>([](const EllipseShape& e) {
        EXPECT_FLOAT_EQ(e.radius_x, 20.0f);
        EXPECT_FLOAT_EQ(e.radius_y, 10.0f);
    });
}

// Test sorted parallel visit
TEST_F(ParallelVisitorTest, ParallelSortedVisit) {
    // Add shapes with specific z-indices
    std::vector<uint32_t> ids;
    for (int i = 0; i < 100; ++i) {
        Circle c(i * 1.0f, 0.0f, 5.0f);
        uint32_t id = container_->add_circle(c);
        container_->set_z_index(id, 100 - i);  // Reverse order
        ids.push_back(id);
    }
    
    // Visit in sorted order
    std::mutex order_mutex;
    std::vector<float> visit_order;
    
    visitor_->visit_sorted(*container_, [&](const auto& shape) {
        using ShapeType = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            std::lock_guard<std::mutex> lock(order_mutex);
            visit_order.push_back(shape.get_center_x());
        }
    });
    
    // Verify sorted order (should be reversed due to z-indices)
    ASSERT_EQ(visit_order.size(), 100u);
    for (size_t i = 1; i < visit_order.size(); ++i) {
        EXPECT_GT(visit_order[i-1], visit_order[i]);
    }
}

// Test work stealing efficiency
TEST_F(ParallelVisitorTest, WorkStealingEfficiency) {
    // Create uneven workload
    const size_t light_work_count = 10000;
    const size_t heavy_work_count = 100;
    
    // Add many shapes with light work
    for (size_t i = 0; i < light_work_count; ++i) {
        Line l(0.0f, 0.0f, 1.0f, 1.0f);
        container_->add_line(l);
    }
    
    // Add few shapes with heavy work
    for (size_t i = 0; i < heavy_work_count; ++i) {
        Circle c(i * 1.0f, i * 1.0f, 5.0f);
        container_->add_circle(c);
    }
    
    // Time parallel execution
    ParallelVisitor::Config config;
    ParallelVisitor visitor(config);
    
    // Heavy work function
    auto heavy_work = [](CircleShape& circle, size_t) {
        // Simulate heavy computation
        volatile float sum = 0.0f;
        for (int i = 0; i < 10000; ++i) {
            sum += circle.center_x * circle.center_y * circle.radius;
        }
    };
    
    // Light work function
    auto light_work = [](LineShape& line, size_t) {
        // Simulate light computation
        volatile float diff = line.x2 - line.x1;
        (void)diff;
    };
    
    // Test parallel execution
    auto start = std::chrono::high_resolution_clock::now();
    visitor.visit_circles(*container_, heavy_work);
    visitor.visit_lines(*container_, light_work);
    auto end = std::chrono::high_resolution_clock::now();
    auto parallel_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Parallel execution: " << parallel_time.count() << " μs\n";
}

// Test thread safety
TEST_F(ParallelVisitorTest, ThreadSafety) {
    // Add shapes
    const size_t count = 1000;
    for (size_t i = 0; i < count; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        Rectangle r(i * 2.0f, i * 3.0f, 5.0f, 5.0f);
        container_->add_circle(c);
        container_->add_rectangle(r);
    }
    
    // Concurrent reads should be safe
    std::atomic<bool> error_occurred(false);
    std::vector<std::thread> threads;
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&]() {
            try {
                for (int iter = 0; iter < 100; ++iter) {
                    visitor_->visit_circles(*container_,
                        [](CircleShape& c, size_t) {
                            // Read-only access
                            volatile float x = c.center_x;
                            volatile float y = c.center_y;
                            volatile float r = c.radius;
                            (void)x; (void)y; (void)r;
                        });
                }
            } catch (...) {
                error_occurred = true;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_FALSE(error_occurred.load());
}

// Test empty container
TEST_F(ParallelVisitorTest, EmptyContainer) {
    // Visit empty container should not crash
    std::atomic<size_t> visit_count(0);
    
    visitor_->visit_circles(*container_,
        [&visit_count](CircleShape&, size_t) {
            visit_count++;
        });
    
    EXPECT_EQ(visit_count.load(), 0u);
    
    // Reduction on empty should return identity
    float sum = visitor_->reduce_circles(
        *container_,
        42.0f,  // identity
        [](const CircleShape& c) { return c.radius; },
        [](float a, float b) { return a + b; }
    );
    
    EXPECT_FLOAT_EQ(sum, 42.0f);
}

// Test different thread counts
TEST_F(ParallelVisitorTest, DifferentThreadCounts) {
    // Add shapes
    const size_t count = 10000;
    for (size_t i = 0; i < count; ++i) {
        Circle c(i * 1.0f, 0.0f, 1.0f);
        container_->add_circle(c);
    }
    
    // Test with different thread counts
    std::vector<size_t> thread_counts = {1, 2, 4, 8};
    
    for (size_t threads : thread_counts) {
        ParallelVisitor::Config config;
        config.thread_count = threads;
        ParallelVisitor visitor(config);
        
        std::atomic<size_t> visit_count(0);
        
        auto start = std::chrono::high_resolution_clock::now();
        visitor.visit_circles(*container_,
            [&visit_count](CircleShape& c, size_t) {
                visit_count++;
                // Simulate some work
                volatile float work = c.center_x * c.center_y;
                (void)work;
            });
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        EXPECT_EQ(visit_count.load(), count);
        std::cout << "Threads: " << threads << ", Time: " << duration.count() << " μs\n";
    }
}

// Test chunk size impact
TEST_F(ParallelVisitorTest, ChunkSizeImpact) {
    // Add shapes
    const size_t count = 100000;
    for (size_t i = 0; i < count; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 1.0f);
        container_->add_circle(c);
    }
    
    // Test different chunk sizes
    std::vector<size_t> chunk_sizes = {16, 64, 256, 1024, 4096};
    
    for (size_t chunk_size : chunk_sizes) {
        ParallelVisitor::Config config;
        config.min_chunk_size = chunk_size;
        // config.max_chunk_size = chunk_size;  // Not available in simple version
        ParallelVisitor visitor(config);
        
        auto start = std::chrono::high_resolution_clock::now();
        visitor.visit_circles(*container_,
            [](CircleShape& c, size_t) {
                // Light work to emphasize overhead
                volatile float x = c.center_x + c.center_y;
                (void)x;
            });
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Chunk size: " << chunk_size 
                  << ", Time: " << duration.count() << " μs\n";
    }
}

// Performance benchmark
TEST_F(ParallelVisitorTest, PerformanceBenchmark) {
    // Large dataset
    const size_t shape_count = 1000000;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    
    std::cout << "Creating " << shape_count << " shapes...\n";
    
    for (size_t i = 0; i < shape_count / 4; ++i) {
        Circle c(dist(rng), dist(rng), std::abs(dist(rng)) * 0.1f);
        Rectangle r(dist(rng), dist(rng), std::abs(dist(rng)), std::abs(dist(rng)));
        Ellipse e(dist(rng), dist(rng), std::abs(dist(rng)) * 0.1f, std::abs(dist(rng)) * 0.1f);
        Line l(dist(rng), dist(rng), dist(rng), dist(rng));
        
        container_->add_circle(c);
        container_->add_rectangle(r);
        container_->add_ellipse(e);
        container_->add_line(l);
    }
    
    // Sequential baseline
    auto start = std::chrono::high_resolution_clock::now();
    float seq_sum = 0.0f;
    container_->for_each_type<CircleShape>([&seq_sum](const CircleShape& c) {
        seq_sum += c.radius;
    });
    auto end = std::chrono::high_resolution_clock::now();
    auto seq_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Parallel version
    start = std::chrono::high_resolution_clock::now();
    float par_sum = visitor_->reduce_circles(
        *container_,
        0.0f,
        [](const CircleShape& c) { return c.radius; },
        [](float a, float b) { return a + b; }
    );
    end = std::chrono::high_resolution_clock::now();
    auto par_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_NEAR(seq_sum, par_sum, seq_sum * 0.0001f);  // Allow 0.01% floating point differences
    
    double speedup = static_cast<double>(seq_time.count()) / par_time.count();
    
    std::cout << "\nPerformance Benchmark Results:\n";
    std::cout << "  Sequential: " << seq_time.count() / 1000.0 << " ms\n";
    std::cout << "  Parallel: " << par_time.count() / 1000.0 << " ms\n";
    std::cout << "  Speedup: " << speedup << "x\n";
    std::cout << "  Thread count: " << std::thread::hardware_concurrency() << "\n";
}