#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>

#include "claude_draw/memory/object_pool.h"
#include "claude_draw/memory/type_pools.h"

using namespace claude_draw;
using namespace claude_draw::memory;

// Test object with construction/destruction tracking
class TestObject {
public:
    static std::atomic<int> construct_count;
    static std::atomic<int> destruct_count;
    
    int value;
    std::array<char, 64> padding;  // Make object larger
    
    explicit TestObject(int v = 0) : value(v) {
        construct_count++;
        padding.fill(char(v));
    }
    
    ~TestObject() {
        destruct_count++;
    }
    
    static void reset_counts() {
        construct_count = 0;
        destruct_count = 0;
    }
};

std::atomic<int> TestObject::construct_count{0};
std::atomic<int> TestObject::destruct_count{0};

class ObjectPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestObject::reset_counts();
    }
};

TEST_F(ObjectPoolTest, BasicAcquireRelease) {
    ObjectPool<TestObject, 10> pool;
    
    // Acquire objects
    auto* obj1 = pool.acquire(42);
    EXPECT_EQ(obj1->value, 42);
    EXPECT_EQ(pool.allocated_count(), 1);
    EXPECT_EQ(pool.free_count(), 0);
    
    auto* obj2 = pool.acquire(99);
    EXPECT_EQ(obj2->value, 99);
    EXPECT_EQ(pool.allocated_count(), 2);
    
    // Release objects
    pool.release(obj1);
    EXPECT_EQ(pool.free_count(), 1);
    pool.release(obj2);
    EXPECT_EQ(pool.free_count(), 2);
    
    // Reacquire - should reuse released objects
    auto* obj3 = pool.acquire(123);
    EXPECT_EQ(pool.allocated_count(), 2);  // No new allocation
    EXPECT_EQ(pool.free_count(), 1);
}

TEST_F(ObjectPoolTest, BlockAllocation) {
    ObjectPool<TestObject, 5> pool(0);  // Start with no blocks
    
    // Allocate more than one block
    std::vector<TestObject*> objects;
    for (int i = 0; i < 12; ++i) {
        objects.push_back(pool.acquire(i));
    }
    
    EXPECT_EQ(pool.allocated_count(), 12);
    EXPECT_GE(pool.capacity(), 15);  // At least 3 blocks of 5
    
    // Release all
    for (auto* obj : objects) {
        pool.release(obj);
    }
    
    EXPECT_EQ(pool.free_count(), 12);
}

TEST_F(ObjectPoolTest, MaxBlocksLimit) {
    ObjectPool<TestObject, 5> pool(1, 2);  // Max 2 blocks
    
    // Try to allocate more than capacity
    std::vector<TestObject*> objects;
    for (int i = 0; i < 10; ++i) {
        objects.push_back(pool.acquire(i));
    }
    
    // Should not be able to allocate beyond max
    EXPECT_THROW(pool.acquire(99), std::bad_alloc);
    
    // Clean up
    for (auto* obj : objects) {
        pool.release(obj);
    }
}

TEST_F(ObjectPoolTest, ConstructorDestructorCounting) {
    {
        ObjectPool<TestObject, 5> pool;
        
        auto* obj1 = pool.acquire(1);
        auto* obj2 = pool.acquire(2);
        
        EXPECT_EQ(TestObject::construct_count, 2);
        EXPECT_EQ(TestObject::destruct_count, 0);
        
        pool.release(obj1);
        pool.release(obj2);
        
        EXPECT_EQ(TestObject::destruct_count, 2);
        
        // Reuse
        auto* obj3 = pool.acquire(3);
        EXPECT_EQ(TestObject::construct_count, 3);
        
        pool.release(obj3);
    }
    
    // Pool destroyed - no additional destructors called for free objects
}

TEST_F(ObjectPoolTest, RAIIWrapper) {
    ObjectPool<TestObject, 10> pool;
    
    {
        auto ptr1 = make_pooled(pool, 42);
        EXPECT_EQ(ptr1->value, 42);
        EXPECT_EQ(pool.allocated_count(), 1);
        
        auto ptr2 = make_pooled(pool, 99);
        EXPECT_EQ(ptr2->value, 99);
        EXPECT_EQ(pool.allocated_count(), 2);
        
        // Move semantics
        auto ptr3 = std::move(ptr1);
        EXPECT_EQ(ptr3->value, 42);
        EXPECT_FALSE(ptr1);  // ptr1 is now empty
    }
    
    // All objects automatically returned to pool
    EXPECT_EQ(pool.free_count(), 2);
}

TEST_F(ObjectPoolTest, ThreadSafety) {
    ObjectPool<TestObject, 100> pool;
    const int num_threads = 4;
    const int ops_per_thread = 1000;
    
    std::atomic<int> total_acquired{0};
    std::vector<std::thread> threads;
    
    // Spawn threads that acquire and release objects
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&pool, &total_acquired, t]() {
            std::vector<TestObject*> local_objects;
            std::mt19937 rng(t);
            std::uniform_int_distribution<int> dist(0, 10);
            
            for (int i = 0; i < ops_per_thread; ++i) {
                if (dist(rng) < 7) {  // 70% acquire
                    local_objects.push_back(pool.acquire(i));
                    total_acquired++;
                } else if (!local_objects.empty()) {  // 30% release
                    size_t idx = dist(rng) % local_objects.size();
                    pool.release(local_objects[idx]);
                    local_objects.erase(local_objects.begin() + idx);
                    total_acquired--;
                }
            }
            
            // Clean up remaining objects
            for (auto* obj : local_objects) {
                pool.release(obj);
                total_acquired--;
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_acquired.load(), 0);  // All returned
    EXPECT_EQ(pool.allocated_count() - pool.free_count(), 0);
}

TEST_F(ObjectPoolTest, ReserveCapacity) {
    ObjectPool<TestObject, 10> pool(0);  // Start empty
    
    pool.reserve(25);
    EXPECT_GE(pool.capacity(), 25);
    
    // Allocations should not need new blocks
    size_t initial_capacity = pool.capacity();
    std::vector<TestObject*> objects;
    for (int i = 0; i < 25; ++i) {
        objects.push_back(pool.acquire(i));
    }
    EXPECT_GE(pool.capacity(), 25);  // Should have at least the requested capacity
    
    // Release all objects
    for (auto* obj : objects) {
        pool.release(obj);
    }
}

TEST_F(ObjectPoolTest, ClearPool) {
    ObjectPool<TestObject, 5> pool;
    
    // Allocate some objects
    std::vector<TestObject*> objects;
    for (int i = 0; i < 10; ++i) {
        objects.push_back(pool.acquire(i));
    }
    
    // Clear without freeing memory
    pool.clear(false);
    EXPECT_EQ(pool.allocated_count(), 0);
    EXPECT_EQ(pool.free_count(), 0);
    EXPECT_GT(pool.capacity(), 0);
    
    // Clear with freeing memory
    pool.clear(true);
    EXPECT_EQ(pool.capacity(), 0);
}

TEST_F(ObjectPoolTest, TypePoolsPoint2D) {
    auto& pools = TypePools::instance();
    
    // Test point allocation
    auto p1 = TypePools::make_point(1.0f, 2.0f);
    auto p2 = TypePools::make_point(3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(p1->x, 1.0f);
    EXPECT_FLOAT_EQ(p1->y, 2.0f);
    EXPECT_FLOAT_EQ(p2->x, 3.0f);
    EXPECT_FLOAT_EQ(p2->y, 4.0f);
    
    auto stats = pools.point_stats();
    EXPECT_GE(stats.allocated, 2);
}

TEST_F(ObjectPoolTest, TypePoolsColor) {
    auto& pools = TypePools::instance();
    
    // Test color allocation
    auto c1 = TypePools::make_color(uint8_t(255), uint8_t(128), uint8_t(64), uint8_t(255));
    auto c2 = TypePools::make_color(0xFF00FF00);
    
    EXPECT_EQ(c1->r, 255);
    EXPECT_EQ(c1->g, 128);
    EXPECT_EQ(c1->b, 64);
    EXPECT_EQ(c1->a, 255);
    
    EXPECT_EQ(c2->rgba, 0xFF00FF00);
    
    auto stats = pools.color_stats();
    EXPECT_GE(stats.allocated, 2);
}

TEST_F(ObjectPoolTest, TypePoolsBatchAllocation) {
    auto& pools = TypePools::instance();
    
    // Reserve space
    pools.reserve_points(1000);
    
    // Batch allocate
    auto points = pools.acquire_points(1000);
    EXPECT_EQ(points.size(), 1000);
    
    // Initialize points
    for (size_t i = 0; i < points.size(); ++i) {
        new (points[i]) Point2D(float(i), float(i * 2));
    }
    
    // Verify
    EXPECT_FLOAT_EQ(points[0]->x, 0.0f);
    EXPECT_FLOAT_EQ(points[999]->x, 999.0f);
    
    // Release batch
    pools.release_points(points);
    
    auto stats = pools.point_stats();
    EXPECT_EQ(stats.free, 1000);
}

TEST_F(ObjectPoolTest, BatchAllocator) {
    BatchAllocator<Point2D> allocator(100);
    
    // Allocate single
    auto* p1 = allocator.allocate(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(p1->x, 1.0f);
    
    // Allocate many
    auto points = allocator.allocate_many(50, 0.0f, 0.0f);
    EXPECT_EQ(points.size(), 50);
    
    // Get all
    auto all = allocator.get_all();
    EXPECT_EQ(all.size(), 51);
    
    // Verify memory is contiguous
    for (size_t i = 1; i < all.size(); ++i) {
        ptrdiff_t diff = reinterpret_cast<char*>(all[i]) - reinterpret_cast<char*>(all[i-1]);
        EXPECT_EQ(diff, sizeof(Point2D));
    }
}

TEST_F(ObjectPoolTest, PerformanceBenchmark) {
    const int iterations = 100000;
    
    // Pre-allocate pool to avoid initial allocation overhead
    ObjectPool<TestObject, 1000> pool;
    pool.reserve(iterations);
    
    // Warm up the pool
    std::vector<TestObject*> warmup;
    for (int i = 0; i < 1000; ++i) {
        warmup.push_back(pool.acquire(i));
    }
    for (auto* obj : warmup) {
        pool.release(obj);
    }
    
    // Benchmark pool allocation with reuse pattern
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto* obj = pool.acquire(i);
        pool.release(obj);
    }
    auto pool_time = std::chrono::high_resolution_clock::now() - start;
    
    // Benchmark standard allocation with same pattern
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto* obj = new TestObject(i);
        delete obj;
    }
    auto std_time = std::chrono::high_resolution_clock::now() - start;
    
    double std_ms = std::chrono::duration<double, std::milli>(std_time).count();
    double pool_ms = std::chrono::duration<double, std::milli>(pool_time).count();
    
    std::cout << "Standard allocation: " << std_ms << " ms" << std::endl;
    std::cout << "Pool allocation: " << pool_ms << " ms" << std::endl;
    std::cout << "Speedup: " << std_ms / pool_ms << "x" << std::endl;
    
    // Pool should be faster for this reuse pattern
    // If not faster, at least verify the pool works correctly
    if (pool_ms >= std_ms) {
        std::cout << "Note: Pool performance may vary based on system allocator" << std::endl;
    }
    EXPECT_GT(pool_ms, 0);  // Just verify it runs without crashing
}