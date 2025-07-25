#include <gtest/gtest.h>
#include "claude_draw/containers/parallel_visitor.h"
#include "claude_draw/containers/soa_container.h"
#include "claude_draw/shapes/circle_optimized.h"
#include "claude_draw/shapes/rectangle_optimized.h"
#include <chrono>
#include <random>
#include <numeric>
#include <atomic>
#include <mutex>
#include <array>

using namespace claude_draw;
using namespace claude_draw::containers;
using namespace claude_draw::shapes;

class ParallelVisitorPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<SoAContainer>();
        rng_.seed(42);
    }
    
    std::unique_ptr<SoAContainer> container_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> coord_dist_{-100.0f, 100.0f};
    std::uniform_real_distribution<float> size_dist_{1.0f, 50.0f};
};

// Test 1: Basic parallel visitor functionality
TEST_F(ParallelVisitorPerformanceTest, BasicParallelIteration) {
    const size_t num_shapes = 10000;
    
    // Add shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 5.0f);
        container_->add_circle(c);
    }
    
    ParallelVisitor visitor;
    std::atomic<size_t> count{0};
    std::atomic<float> sum_x{0.0f};
    
    // Visit all circles in parallel
    visitor.visit_circles(*container_, [&count, &sum_x](CircleShape& circle, size_t index) {
        count.fetch_add(1, std::memory_order_relaxed);
        
        // Atomic float addition using compare-exchange
        float current_x = sum_x.load(std::memory_order_relaxed);
        float new_x;
        do {
            new_x = current_x + circle.center_x;
        } while (!sum_x.compare_exchange_weak(current_x, new_x, std::memory_order_relaxed));
    });
    
    EXPECT_EQ(count.load(), num_shapes);
    
    // Calculate expected sum
    float expected_sum = 0.0f;
    for (size_t i = 0; i < num_shapes; ++i) {
        expected_sum += i * 0.1f;
    }
    
    EXPECT_NEAR(sum_x.load(), expected_sum, 0.1f);
}

// Test 2: Parallel vs Sequential performance
TEST_F(ParallelVisitorPerformanceTest, ParallelVsSequentialPerformance) {
    const size_t num_shapes = 1000000;  // 1 million shapes
    
    // Add shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_));
        container_->add_circle(c);
    }
    
    // Sequential processing
    auto seq_start = std::chrono::high_resolution_clock::now();
    
    double seq_area_sum = 0.0;
    const auto& circles = container_->get_circles();
    for (const auto& circle : circles) {
        seq_area_sum += M_PI * circle.radius * circle.radius;
    }
    
    auto seq_end = std::chrono::high_resolution_clock::now();
    auto seq_duration = std::chrono::duration_cast<std::chrono::microseconds>(seq_end - seq_start);
    
    // Parallel processing with thread-local accumulation
    ParallelVisitor visitor;
    constexpr size_t max_threads = 64;
    std::array<std::atomic<double>, max_threads> thread_sums{};
    std::atomic<size_t> thread_counter{0};
    
    auto par_start = std::chrono::high_resolution_clock::now();
    
    visitor.visit_circles(*container_, [&thread_sums, &thread_counter](CircleShape& circle, size_t index) {
        thread_local size_t my_thread_id = thread_counter.fetch_add(1) % max_threads;
        double area = M_PI * circle.radius * circle.radius;
        
        // Accumulate in thread-local slot
        double current = thread_sums[my_thread_id].load(std::memory_order_relaxed);
        thread_sums[my_thread_id].store(current + area, std::memory_order_relaxed);
    });
    
    // Sum up thread-local results
    double par_area_sum = 0.0;
    for (auto& sum : thread_sums) {
        par_area_sum += sum.load();
    }
    
    auto par_end = std::chrono::high_resolution_clock::now();
    auto par_duration = std::chrono::duration_cast<std::chrono::microseconds>(par_end - par_start);
    
    // Verify results match
    EXPECT_NEAR(seq_area_sum, par_area_sum, seq_area_sum * 0.001);  // 0.1% tolerance
    
    // Calculate speedup
    double speedup = static_cast<double>(seq_duration.count()) / par_duration.count();
    
    std::cout << "Parallel visitor performance:" << std::endl;
    std::cout << "  Shapes: " << num_shapes << std::endl;
    std::cout << "  Sequential time: " << seq_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Parallel time: " << par_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Speedup: " << speedup << "x" << std::endl;
    std::cout << "  Threads: " << std::thread::hardware_concurrency() << std::endl;
    
    // On multi-core systems, expect some speedup
    // Note: Due to atomic operations overhead, speedup may be modest
    if (std::thread::hardware_concurrency() > 1) {
        EXPECT_GT(speedup, 1.0);  // Any speedup is good given the simple workload
    }
}

// Test 3: Parallel reduction
TEST_F(ParallelVisitorPerformanceTest, ParallelReduction) {
    const size_t num_shapes = 100000;
    
    // Add circles with known radii
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(0.0f, 0.0f, static_cast<float>(i + 1));
        container_->add_circle(c);
    }
    
    ParallelVisitor visitor;
    
    // Find maximum radius using parallel reduction
    auto start = std::chrono::high_resolution_clock::now();
    
    float max_radius = visitor.reduce_circles(
        *container_,
        0.0f,  // identity
        [](const CircleShape& circle) { return circle.radius; },  // extract radius
        [](float a, float b) { return std::max(a, b); }  // combine with max
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_FLOAT_EQ(max_radius, static_cast<float>(num_shapes));
    
    std::cout << "Parallel reduction performance:" << std::endl;
    std::cout << "  Shapes: " << num_shapes << std::endl;
    std::cout << "  Time: " << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Max radius found: " << max_radius << std::endl;
}

// Test 4: Mixed shape type processing
TEST_F(ParallelVisitorPerformanceTest, MixedShapeProcessing) {
    const size_t shapes_per_type = 25000;
    
    // Add different shape types
    for (size_t i = 0; i < shapes_per_type; ++i) {
        Circle c(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_));
        Rectangle r(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_), size_dist_(rng_));
        Ellipse e(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_), size_dist_(rng_));
        Line l(coord_dist_(rng_), coord_dist_(rng_), coord_dist_(rng_), coord_dist_(rng_));
        
        container_->add_circle(c);
        container_->add_rectangle(r);
        container_->add_ellipse(e);
        container_->add_line(l);
    }
    
    ParallelVisitor visitor;
    
    // Process all shape types in parallel
    std::atomic<size_t> total_count{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Launch parallel processing for each type
    std::vector<std::thread> threads;
    
    threads.emplace_back([&]() {
        visitor.visit_circles(*container_, [&total_count](CircleShape& circle, size_t index) {
            total_count.fetch_add(1, std::memory_order_relaxed);
        });
    });
    
    threads.emplace_back([&]() {
        visitor.visit_rectangles(*container_, [&total_count](RectangleShape& rect, size_t index) {
            total_count.fetch_add(1, std::memory_order_relaxed);
        });
    });
    
    threads.emplace_back([&]() {
        visitor.visit_ellipses(*container_, [&total_count](EllipseShape& ellipse, size_t index) {
            total_count.fetch_add(1, std::memory_order_relaxed);
        });
    });
    
    threads.emplace_back([&]() {
        visitor.visit_lines(*container_, [&total_count](LineShape& line, size_t index) {
            total_count.fetch_add(1, std::memory_order_relaxed);
        });
    });
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_EQ(total_count.load(), shapes_per_type * 4);
    
    std::cout << "Mixed shape processing:" << std::endl;
    std::cout << "  Total shapes: " << shapes_per_type * 4 << std::endl;
    std::cout << "  Time: " << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Shapes/ms: " << (shapes_per_type * 4) / (duration.count() / 1000.0) << std::endl;
}

// Test 5: Parallel bounds calculation
TEST_F(ParallelVisitorPerformanceTest, ParallelBoundsCalculation) {
    const size_t num_shapes = 100000;
    
    // Add shapes distributed across space
    for (size_t i = 0; i < num_shapes; ++i) {
        float angle = (i * 2.0f * M_PI) / num_shapes;
        float radius = 100.0f + (i % 1000) * 0.1f;
        
        Circle c(radius * std::cos(angle), radius * std::sin(angle), 5.0f);
        Rectangle r(radius * std::cos(angle + 0.1f), radius * std::sin(angle + 0.1f), 10.0f, 15.0f);
        
        if (i % 2 == 0) {
            container_->add_circle(c);
        } else {
            container_->add_rectangle(r);
        }
    }
    
    ParallelVisitor visitor;
    
    // Time parallel bounds calculation
    auto start = std::chrono::high_resolution_clock::now();
    BoundingBox parallel_bounds = visitor.calculate_bounds_parallel(*container_);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto parallel_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Compare with sequential calculation
    start = std::chrono::high_resolution_clock::now();
    BoundingBox sequential_bounds = container_->get_bounds();
    end = std::chrono::high_resolution_clock::now();
    
    auto sequential_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Verify bounds match
    EXPECT_NEAR(parallel_bounds.min_x, sequential_bounds.min_x, 0.001f);
    EXPECT_NEAR(parallel_bounds.max_x, sequential_bounds.max_x, 0.001f);
    EXPECT_NEAR(parallel_bounds.min_y, sequential_bounds.min_y, 0.001f);
    EXPECT_NEAR(parallel_bounds.max_y, sequential_bounds.max_y, 0.001f);
    
    std::cout << "Parallel bounds calculation:" << std::endl;
    std::cout << "  Shapes: " << num_shapes << std::endl;
    std::cout << "  Parallel time: " << parallel_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Sequential time: " << sequential_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Speedup: " << static_cast<double>(sequential_duration.count()) / parallel_duration.count() << "x" << std::endl;
}

// Test 6: Thread safety
TEST_F(ParallelVisitorPerformanceTest, ThreadSafety) {
    const size_t num_shapes = 10000;
    const size_t num_iterations = 100;
    
    // Add shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 5.0f);
        container_->add_circle(c);
    }
    
    ParallelVisitor visitor;
    
    // Run multiple parallel iterations to stress test thread safety
    for (size_t iter = 0; iter < num_iterations; ++iter) {
        std::atomic<size_t> count{0};
        std::vector<std::atomic<bool>> visited(num_shapes);
        
        for (auto& v : visited) {
            v.store(false);
        }
        
        visitor.visit_circles(*container_, [&count, &visited](CircleShape& circle, size_t index) {
            // Mark as visited
            bool expected = false;
            bool success = visited[index].compare_exchange_strong(expected, true);
            EXPECT_TRUE(success) << "Shape " << index << " visited multiple times!";
            
            count.fetch_add(1, std::memory_order_relaxed);
        });
        
        EXPECT_EQ(count.load(), num_shapes);
        
        // Verify all shapes were visited exactly once
        for (size_t i = 0; i < num_shapes; ++i) {
            EXPECT_TRUE(visited[i].load()) << "Shape " << i << " not visited";
        }
    }
}

// Test 7: Custom thread configuration
TEST_F(ParallelVisitorPerformanceTest, CustomThreadConfiguration) {
    const size_t num_shapes = 100000;
    
    // Add shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_));
        container_->add_circle(c);
    }
    
    // Test with different thread counts
    std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};
    
    for (size_t thread_count : thread_counts) {
        if (thread_count > std::thread::hardware_concurrency()) {
            continue;  // Skip if more than hardware threads
        }
        
        ParallelVisitor::Config config;
        config.thread_count = thread_count;
        config.min_chunk_size = 1000;
        
        ParallelVisitor visitor(config);
        
        std::atomic<size_t> count{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        visitor.visit_circles(*container_, [&count](CircleShape& circle, size_t index) {
            // Simulate some work
            volatile float sum = 0.0f;
            for (int i = 0; i < 100; ++i) {
                sum += circle.radius * i;
            }
            count.fetch_add(1, std::memory_order_relaxed);
        });
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        EXPECT_EQ(count.load(), num_shapes);
        
        std::cout << "Thread count " << thread_count << ": " 
                  << duration.count() / 1000.0 << " ms" << std::endl;
    }
}

// Test 8: Chunk size impact
TEST_F(ParallelVisitorPerformanceTest, ChunkSizeImpact) {
    const size_t num_shapes = 100000;
    
    // Add shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 5.0f);
        container_->add_circle(c);
    }
    
    // Test different chunk sizes
    std::vector<size_t> chunk_sizes = {1, 10, 100, 1000, 10000};
    
    for (size_t chunk_size : chunk_sizes) {
        ParallelVisitor::Config config;
        config.min_chunk_size = chunk_size;
        
        ParallelVisitor visitor(config);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::atomic<size_t> count{0};
        visitor.visit_circles(*container_, [&count](CircleShape& circle, size_t index) {
            count.fetch_add(1, std::memory_order_relaxed);
        });
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        EXPECT_EQ(count.load(), num_shapes);
        
        std::cout << "Chunk size " << chunk_size << ": " 
                  << duration.count() / 1000.0 << " ms" << std::endl;
    }
}

// Test 9: Work stealing simulation (simplified)
TEST_F(ParallelVisitorPerformanceTest, LoadBalancing) {
    const size_t num_shapes = 100000;
    
    // Add shapes with varying processing costs
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(coord_dist_(rng_), coord_dist_(rng_), size_dist_(rng_));
        container_->add_circle(c);
    }
    
    ParallelVisitor visitor;
    
    // Track work distribution
    std::vector<std::atomic<size_t>> thread_work_counts(16);
    for (auto& count : thread_work_counts) {
        count.store(0);
    }
    
    std::atomic<size_t> next_thread_id{0};
    thread_local size_t thread_id = 0;
    thread_local bool thread_id_set = false;
    
    visitor.visit_circles(*container_, [&thread_work_counts, &next_thread_id](CircleShape& circle, size_t index) {
        if (!thread_id_set) {
            thread_id = next_thread_id.fetch_add(1);
            thread_id_set = true;
        }
        
        // Simulate variable work (some shapes take longer)
        volatile float sum = 0.0f;
        int iterations = (index % 100 == 0) ? 1000 : 10;  // Some shapes take 100x longer
        
        for (int i = 0; i < iterations; ++i) {
            sum += circle.radius * i;
        }
        
        if (thread_id < thread_work_counts.size()) {
            thread_work_counts[thread_id].fetch_add(1, std::memory_order_relaxed);
        }
    });
    
    // Analyze work distribution
    size_t total_work = 0;
    size_t active_threads = 0;
    
    std::cout << "Work distribution:" << std::endl;
    for (size_t i = 0; i < thread_work_counts.size(); ++i) {
        size_t count = thread_work_counts[i].load();
        if (count > 0) {
            std::cout << "  Thread " << i << ": " << count << " shapes" << std::endl;
            total_work += count;
            active_threads++;
        }
    }
    
    EXPECT_EQ(total_work, num_shapes);
    
    // Calculate load balance factor
    if (active_threads > 0) {
        double avg_work = static_cast<double>(total_work) / active_threads;
        double max_deviation = 0.0;
        
        for (size_t i = 0; i < active_threads; ++i) {
            double deviation = std::abs(thread_work_counts[i].load() - avg_work) / avg_work;
            max_deviation = std::max(max_deviation, deviation);
        }
        
        std::cout << "Load balance factor: " << (1.0 - max_deviation) << std::endl;
        
        // Expect reasonable load balance (within 50% of average)
        EXPECT_LT(max_deviation, 0.5);
    }
}

// Test 10: Scalability test
TEST_F(ParallelVisitorPerformanceTest, Scalability) {
    std::vector<size_t> shape_counts = {1000, 10000, 100000, 1000000};
    
    for (size_t count : shape_counts) {
        // Clear container
        container_->clear();
        
        // Add shapes
        for (size_t i = 0; i < count; ++i) {
            Circle c(i * 0.1f, i * 0.2f, 5.0f);
            container_->add_circle(c);
        }
        
        ParallelVisitor visitor;
        
        // Time parallel processing with thread-local accumulation
        auto start = std::chrono::high_resolution_clock::now();
        
        constexpr size_t max_threads = 64;
        std::array<std::atomic<double>, max_threads> thread_sums{};
        std::atomic<size_t> thread_counter{0};
        
        visitor.visit_circles(*container_, [&thread_sums, &thread_counter](CircleShape& circle, size_t index) {
            thread_local size_t my_thread_id = thread_counter.fetch_add(1) % max_threads;
            double local_area = M_PI * circle.radius * circle.radius;
            
            // Accumulate in thread-local slot
            double current = thread_sums[my_thread_id].load(std::memory_order_relaxed);
            thread_sums[my_thread_id].store(current + local_area, std::memory_order_relaxed);
        });
        
        // Sum up results
        double total_sum = 0.0;
        for (auto& sum : thread_sums) {
            total_sum += sum.load();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double ms_per_million = (duration.count() / 1000.0) / (count / 1000000.0);
        
        std::cout << "Scalability - " << count << " shapes: " 
                  << duration.count() / 1000.0 << " ms (" 
                  << ms_per_million << " ms per million)" << std::endl;
        
        // Verify linear or better scalability
        // Note: With atomic operations, 200ms per million is reasonable
        if (count >= 10000) {
            EXPECT_LT(ms_per_million, 200.0);  // Should process 1M shapes in < 200ms
        }
    }
}