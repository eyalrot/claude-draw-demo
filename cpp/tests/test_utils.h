#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <memory>
#include <random>

namespace claude_draw::test {

// Performance testing utilities
class PerformanceTimer {
public:
    PerformanceTimer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
    double elapsed_us() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(end - start_).count();
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_;
};

// Memory usage tracking
class MemoryTracker {
public:
    struct Stats {
        size_t allocations = 0;
        size_t deallocations = 0;
        size_t bytes_allocated = 0;
        size_t bytes_deallocated = 0;
        size_t peak_bytes = 0;
    };
    
    void record_allocation(size_t bytes) {
        stats_.allocations++;
        stats_.bytes_allocated += bytes;
        current_bytes_ += bytes;
        if (current_bytes_ > stats_.peak_bytes) {
            stats_.peak_bytes = current_bytes_;
        }
    }
    
    void record_deallocation(size_t bytes) {
        stats_.deallocations++;
        stats_.bytes_deallocated += bytes;
        current_bytes_ -= bytes;
    }
    
    const Stats& get_stats() const { return stats_; }
    void reset() { stats_ = Stats{}; current_bytes_ = 0; }
    
private:
    Stats stats_;
    size_t current_bytes_ = 0;
};

// Test data generators
class TestDataGenerator {
public:
    TestDataGenerator(unsigned seed = std::random_device{}()) 
        : rng_(seed) {}
    
    float random_float(float min = -1000.0f, float max = 1000.0f) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng_);
    }
    
    int random_int(int min = 0, int max = 1000) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng_);
    }
    
    uint32_t random_color() {
        std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
        return dist(rng_);
    }
    
private:
    std::mt19937 rng_;
};

// Common test fixtures
class ClaudeDrawTest : public ::testing::Test {
protected:
    void SetUp() override {
        generator_ = std::make_unique<TestDataGenerator>();
    }
    
    void TearDown() override {
        // Common cleanup
    }
    
    std::unique_ptr<TestDataGenerator> generator_;
};

// Performance test fixture
class PerformanceTest : public ClaudeDrawTest {
protected:
    static constexpr size_t WARM_UP_ITERATIONS = 100;
    static constexpr size_t BENCHMARK_ITERATIONS = 10000;
    
    template<typename Func>
    double benchmark(Func func, size_t iterations = BENCHMARK_ITERATIONS) {
        // Warm up
        for (size_t i = 0; i < WARM_UP_ITERATIONS; ++i) {
            func();
        }
        
        // Actual benchmark
        PerformanceTimer timer;
        for (size_t i = 0; i < iterations; ++i) {
            func();
        }
        
        return timer.elapsed_ms() / iterations;
    }
};

// Floating point comparison helpers
constexpr float EPSILON = 1e-6f;

inline bool nearly_equal(float a, float b, float epsilon = EPSILON) {
    return std::abs(a - b) < epsilon;
}

// Custom matchers for better test output
MATCHER_P2(FloatNear, expected, epsilon, "") {
    return nearly_equal(arg, expected, epsilon);
}

MATCHER_P(PointNear, expected, "") {
    // Assuming Point2D has x and y members
    return nearly_equal(arg.x, expected.x) && 
           nearly_equal(arg.y, expected.y);
}

} // namespace claude_draw::test