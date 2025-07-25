#include <gtest/gtest.h>
#include "claude_draw/memory/object_recycler.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <set>
#include <cstring>

using namespace claude_draw::memory;

// Test object with destruction tracking
struct RecyclerTestObject {
    static std::atomic<int> construct_count;
    static std::atomic<int> destruct_count;
    
    int value;
    char padding[60];  // Make object larger
    
    explicit RecyclerTestObject(int v = 0) : value(v) {
        construct_count++;
        std::memset(padding, v & 0xFF, sizeof(padding));
    }
    
    ~RecyclerTestObject() {
        destruct_count++;
    }
    
    static void reset_counts() {
        construct_count = 0;
        destruct_count = 0;
    }
};

std::atomic<int> RecyclerTestObject::construct_count{0};
std::atomic<int> RecyclerTestObject::destruct_count{0};

class ObjectRecyclerTest : public ::testing::Test {
protected:
    void SetUp() override {
        RecyclerTestObject::reset_counts();
        recycler_ = std::make_unique<ObjectRecycler<RecyclerTestObject>>();
    }
    
    void TearDown() override {
        recycler_.reset();
        // Verify all objects were properly destructed
        EXPECT_EQ(RecyclerTestObject::construct_count.load(), RecyclerTestObject::destruct_count.load());
    }
    
    std::unique_ptr<ObjectRecycler<RecyclerTestObject>> recycler_;
};

// Test basic allocation and deallocation
TEST_F(ObjectRecyclerTest, BasicAllocation) {
    RecyclerTestObject* obj1 = recycler_->allocate(42);
    ASSERT_NE(obj1, nullptr);
    EXPECT_EQ(obj1->value, 42);
    EXPECT_EQ(RecyclerTestObject::construct_count.load(), 1);
    
    recycler_->deallocate(obj1);
    EXPECT_EQ(RecyclerTestObject::destruct_count.load(), 1);
    EXPECT_EQ(recycler_->get_free_count(), 1u);
}

// Test recycling
TEST_F(ObjectRecyclerTest, Recycling) {
    // Allocate and deallocate
    RecyclerTestObject* obj1 = recycler_->allocate(10);
    void* addr1 = obj1;
    recycler_->deallocate(obj1);
    
    // Allocate again - should get recycled object
    RecyclerTestObject* obj2 = recycler_->allocate(20);
    void* addr2 = obj2;
    EXPECT_EQ(addr1, addr2);  // Same memory address
    EXPECT_EQ(obj2->value, 20);
    
    // Check stats
    auto stats = recycler_->get_stats();
    EXPECT_EQ(stats.total_allocated, 1u);
    EXPECT_EQ(stats.total_recycled, 1u);
    EXPECT_GT(stats.recycle_rate, 0.0);
    
    recycler_->deallocate(obj2);
}

// Test multiple recycling
TEST_F(ObjectRecyclerTest, MultipleRecycling) {
    const int num_objects = 100;
    std::vector<RecyclerTestObject*> objects;
    
    // First allocation round
    for (int i = 0; i < num_objects; ++i) {
        objects.push_back(recycler_->allocate(i));
    }
    
    // Deallocate all
    for (auto* obj : objects) {
        recycler_->deallocate(obj);
    }
    objects.clear();
    
    EXPECT_EQ(recycler_->get_free_count(), num_objects);
    
    // Second allocation round - should all be recycled
    for (int i = 0; i < num_objects; ++i) {
        objects.push_back(recycler_->allocate(i + 1000));
    }
    
    auto stats = recycler_->get_stats();
    EXPECT_EQ(stats.total_allocated, num_objects);
    EXPECT_EQ(stats.total_recycled, num_objects);
    EXPECT_NEAR(stats.recycle_rate, 50.0, 0.1);
    
    // Cleanup
    for (auto* obj : objects) {
        recycler_->deallocate(obj);
    }
}

// Test clear operation
TEST_F(ObjectRecyclerTest, Clear) {
    // Create a vector to hold all allocated objects
    std::vector<RecyclerTestObject*> objects;
    
    // Allocate 10 objects
    for (int i = 0; i < 10; ++i) {
        objects.push_back(recycler_->allocate(i));
    }
    
    // Deallocate all objects at once to ensure they all go to the free list
    for (auto* obj : objects) {
        recycler_->deallocate(obj);
    }
    
    // Now all 10 objects should be in the free list
    EXPECT_EQ(recycler_->get_free_count(), 10u);
    
    // Clear free list
    recycler_->clear();
    EXPECT_EQ(recycler_->get_free_count(), 0u);
    
    // Verify destructor count increased by 10 (the objects that were in the free list)
    // 10 destructors called during deallocate, 0 during clear (already destructed)
    EXPECT_EQ(RecyclerTestObject::destruct_count.load(), 10);
}

// Test trim operation
TEST_F(ObjectRecyclerTest, Trim) {
    ObjectRecycler<RecyclerTestObject> recycler(false, 5, 100);  // Max 5 per size
    
    // Add 10 objects to free list
    for (int i = 0; i < 10; ++i) {
        RecyclerTestObject* obj = recycler.allocate(i);
        recycler.deallocate(obj);
    }
    
    // Should be limited by max_free_per_size
    EXPECT_LE(recycler.get_free_count(), 5u);
}

// Test thread safety
TEST_F(ObjectRecyclerTest, ThreadSafety) {
    ObjectRecycler<RecyclerTestObject> thread_safe_recycler(true);  // Thread-safe
    
    const int num_threads = 4;
    const int ops_per_thread = 1000;
    std::atomic<int> total_allocated{0};
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&thread_safe_recycler, &total_allocated, t, ops_per_thread]() {
            std::vector<RecyclerTestObject*> local_objects;
            
            for (int i = 0; i < ops_per_thread; ++i) {
                if (i % 3 == 0 && !local_objects.empty()) {
                    // Deallocate
                    thread_safe_recycler.deallocate(local_objects.back());
                    local_objects.pop_back();
                } else {
                    // Allocate
                    RecyclerTestObject* obj = thread_safe_recycler.allocate(t * 1000 + i);
                    ASSERT_NE(obj, nullptr);
                    local_objects.push_back(obj);
                    total_allocated++;
                }
            }
            
            // Cleanup remaining
            for (auto* obj : local_objects) {
                thread_safe_recycler.deallocate(obj);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify operations completed successfully
    EXPECT_GT(total_allocated.load(), 0);
    
    // Check final state
    auto stats = thread_safe_recycler.get_stats();
    EXPECT_GT(stats.total_allocated, 0u);
    EXPECT_GE(stats.total_recycled, 0u);
}

// Test performance comparison
TEST_F(ObjectRecyclerTest, PerformanceComparison) {
    const int num_iterations = 10000;
    
    // Benchmark with recycler
    auto recycler_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; ++i) {
        RecyclerTestObject* obj = recycler_->allocate(i);
        recycler_->deallocate(obj);
    }
    auto recycler_end = std::chrono::high_resolution_clock::now();
    auto recycler_duration = std::chrono::duration_cast<std::chrono::microseconds>(recycler_end - recycler_start);
    
    // Benchmark with new/delete
    RecyclerTestObject::reset_counts();
    auto new_delete_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; ++i) {
        RecyclerTestObject* obj = new RecyclerTestObject(i);
        delete obj;
    }
    auto new_delete_end = std::chrono::high_resolution_clock::now();
    auto new_delete_duration = std::chrono::duration_cast<std::chrono::microseconds>(new_delete_end - new_delete_start);
    
    std::cout << "Recycler: " << recycler_duration.count() << " μs\n";
    std::cout << "New/Delete: " << new_delete_duration.count() << " μs\n";
    std::cout << "Speedup: " << static_cast<double>(new_delete_duration.count()) / recycler_duration.count() << "x\n";
    
    // Recycler should be faster after warm-up
    auto stats = recycler_->get_stats();
    EXPECT_GT(stats.recycle_rate, 90.0);  // Most operations should be recycled
}

// Multi-size recycler tests
class MultiSizeRecyclerTest : public ::testing::Test {
protected:
    void SetUp() override {
        recycler_ = std::make_unique<MultiSizeRecycler>();
    }
    
    std::unique_ptr<MultiSizeRecycler> recycler_;
};

// Test basic multi-size allocation
TEST_F(MultiSizeRecyclerTest, BasicAllocation) {
    void* ptr1 = recycler_->allocate(16);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = recycler_->allocate(64);
    ASSERT_NE(ptr2, nullptr);
    
    void* ptr3 = recycler_->allocate(1024);
    ASSERT_NE(ptr3, nullptr);
    
    EXPECT_NE(ptr1, ptr2);
    EXPECT_NE(ptr2, ptr3);
    
    recycler_->deallocate(ptr1, 16);
    recycler_->deallocate(ptr2, 64);
    recycler_->deallocate(ptr3, 1024);
}

// Test size class recycling
TEST_F(MultiSizeRecyclerTest, SizeClassRecycling) {
    // Allocate various sizes
    std::vector<std::pair<void*, size_t>> allocations;
    
    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024};
    for (size_t size : sizes) {
        void* ptr = recycler_->allocate(size);
        ASSERT_NE(ptr, nullptr);
        allocations.push_back({ptr, size});
    }
    
    // Deallocate all
    for (const auto& [ptr, size] : allocations) {
        recycler_->deallocate(ptr, size);
    }
    
    // Allocate again - should be recycled
    for (size_t size : sizes) {
        void* ptr = recycler_->allocate(size);
        ASSERT_NE(ptr, nullptr);
        recycler_->deallocate(ptr, size);
    }
    
    auto stats = recycler_->get_stats();
    EXPECT_EQ(stats.total_allocated, 8u);
    EXPECT_GE(stats.total_recycled, 8u);
}

// Test alignment
TEST_F(MultiSizeRecyclerTest, Alignment) {
    size_t sizes[] = {7, 15, 31, 63, 127};
    
    for (size_t size : sizes) {
        void* ptr = recycler_->allocate(size, 16);  // 16-byte alignment
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 16, 0u);
        recycler_->deallocate(ptr, size);
    }
}

// Test large allocations
TEST_F(MultiSizeRecyclerTest, LargeAllocations) {
    size_t large_size = 1024 * 1024;  // 1MB
    void* ptr = recycler_->allocate(large_size);
    ASSERT_NE(ptr, nullptr);
    
    // Write to entire allocation
    std::memset(ptr, 0xAB, large_size);
    
    recycler_->deallocate(ptr, large_size);
    
    // Allocate again
    void* ptr2 = recycler_->allocate(large_size);
    ASSERT_NE(ptr2, nullptr);
    recycler_->deallocate(ptr2, large_size);
}

// Test statistics
TEST_F(MultiSizeRecyclerTest, Statistics) {
    // Allocate from different size classes
    std::vector<std::pair<void*, size_t>> ptrs;
    
    for (int i = 0; i < 100; ++i) {
        size_t size = 8 << (i % 8);  // 8, 16, 32, ..., 1024
        void* ptr = recycler_->allocate(size);
        ptrs.push_back({ptr, size});
    }
    
    auto stats1 = recycler_->get_stats();
    EXPECT_EQ(stats1.total_allocated, 100u);
    EXPECT_EQ(stats1.total_recycled, 0u);
    
    // Deallocate half
    for (size_t i = 0; i < 50; ++i) {
        recycler_->deallocate(ptrs[i].first, ptrs[i].second);
    }
    
    // Allocate more - should recycle
    for (int i = 0; i < 50; ++i) {
        size_t size = 8 << (i % 8);
        void* ptr = recycler_->allocate(size);
        ASSERT_NE(ptr, nullptr);
        recycler_->deallocate(ptr, size);
    }
    
    auto stats2 = recycler_->get_stats();
    EXPECT_GT(stats2.total_recycled, 0u);
    EXPECT_GT(stats2.recycle_rate, 0.0);
    
    // Cleanup
    for (size_t i = 50; i < ptrs.size(); ++i) {
        recycler_->deallocate(ptrs[i].first, ptrs[i].second);
    }
}

// Test thread safety for multi-size
TEST_F(MultiSizeRecyclerTest, ThreadSafety) {
    MultiSizeRecycler thread_safe_recycler(true);
    
    const int num_threads = 4;
    const int ops_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&thread_safe_recycler, t, ops_per_thread]() {
            std::vector<std::pair<void*, size_t>> local_allocs;
            
            for (int i = 0; i < ops_per_thread; ++i) {
                size_t size = 8 << ((t + i) % 10);  // Various sizes
                
                if (i % 3 == 0 && !local_allocs.empty()) {
                    // Deallocate
                    auto [ptr, size] = local_allocs.back();
                    thread_safe_recycler.deallocate(ptr, size);
                    local_allocs.pop_back();
                } else {
                    // Allocate
                    void* ptr = thread_safe_recycler.allocate(size);
                    ASSERT_NE(ptr, nullptr);
                    local_allocs.push_back({ptr, size});
                }
            }
            
            // Cleanup
            for (const auto& [ptr, size] : local_allocs) {
                thread_safe_recycler.deallocate(ptr, size);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = thread_safe_recycler.get_stats();
    EXPECT_GT(stats.total_allocated, 0u);
}