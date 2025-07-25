#include <gtest/gtest.h>
#include "claude_draw/memory/arena_allocator.h"
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>

using namespace claude_draw::memory;

class ArenaAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        arena_ = std::make_unique<ArenaAllocator>();
    }
    
    std::unique_ptr<ArenaAllocator> arena_;
};

// Test basic allocation
TEST_F(ArenaAllocatorTest, BasicAllocation) {
    void* ptr1 = arena_->allocate(64);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = arena_->allocate(128);
    ASSERT_NE(ptr2, nullptr);
    
    // Pointers should be different
    EXPECT_NE(ptr1, ptr2);
    
    // Write to allocated memory
    std::memset(ptr1, 0xAA, 64);
    std::memset(ptr2, 0xBB, 128);
}

// Test alignment
TEST_F(ArenaAllocatorTest, Alignment) {
    void* ptr1 = arena_->allocate(17, 16);  // 16-byte alignment
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 16, 0u);
    
    void* ptr2 = arena_->allocate(33, 32);  // 32-byte alignment
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 32, 0u);
    
    void* ptr3 = arena_->allocate(7, 64);   // 64-byte alignment
    ASSERT_NE(ptr3, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr3) % 64, 0u);
}

// Test object creation
TEST_F(ArenaAllocatorTest, ObjectCreation) {
    struct TestObject {
        int a;
        double b;
        char c[100];
        
        TestObject(int x, double y) : a(x), b(y) {
            std::memset(c, 0, sizeof(c));
        }
    };
    
    TestObject* obj1 = arena_->create<TestObject>(42, 3.14);
    ASSERT_NE(obj1, nullptr);
    EXPECT_EQ(obj1->a, 42);
    EXPECT_DOUBLE_EQ(obj1->b, 3.14);
    
    TestObject* obj2 = arena_->create<TestObject>(100, 2.71);
    ASSERT_NE(obj2, nullptr);
    EXPECT_EQ(obj2->a, 100);
    EXPECT_DOUBLE_EQ(obj2->b, 2.71);
}

// Test array allocation
TEST_F(ArenaAllocatorTest, ArrayAllocation) {
    int* arr = arena_->allocate_array<int>(100);
    ASSERT_NE(arr, nullptr);
    
    // Write to array
    for (int i = 0; i < 100; ++i) {
        arr[i] = i * i;
    }
    
    // Verify values
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(arr[i], i * i);
    }
}

// Test reset functionality
TEST_F(ArenaAllocatorTest, Reset) {
    size_t initial_allocated = arena_->get_allocated_size();
    
    // Allocate some memory
    arena_->allocate(1024);
    arena_->allocate(2048);
    arena_->allocate(4096);
    
    EXPECT_GT(arena_->get_allocated_size(), initial_allocated);
    
    // Reset arena
    arena_->reset();
    
    // Allocated size should be back to 0
    EXPECT_EQ(arena_->get_allocated_size(), 0u);
    
    // Should be able to allocate again
    void* ptr = arena_->allocate(512);
    ASSERT_NE(ptr, nullptr);
}

// Test large allocations
TEST_F(ArenaAllocatorTest, LargeAllocations) {
    // Allocate larger than default block size
    size_t large_size = ArenaAllocator::DEFAULT_BLOCK_SIZE * 2;
    void* large_ptr = arena_->allocate(large_size);
    ASSERT_NE(large_ptr, nullptr);
    
    // Should be able to write to entire allocation
    std::memset(large_ptr, 0xCC, large_size);
    
    // Check that block count increased
    EXPECT_GE(arena_->get_block_count(), 2u);
}

// Test statistics
TEST_F(ArenaAllocatorTest, Statistics) {
    auto stats1 = arena_->get_stats();
    EXPECT_EQ(stats1.allocated_bytes, 0u);
    EXPECT_GT(stats1.capacity_bytes, 0u);
    EXPECT_GE(stats1.block_count, 1u);
    
    // Allocate some memory
    arena_->allocate(1000);
    arena_->allocate(2000);
    arena_->allocate(3000);
    
    auto stats2 = arena_->get_stats();
    EXPECT_GE(stats2.allocated_bytes, 6000u);
    EXPECT_GT(stats2.utilization_percent, 0.0);
    EXPECT_LE(stats2.utilization_percent, 100.0);
}

// Test thread safety
TEST_F(ArenaAllocatorTest, ThreadSafety) {
    ArenaAllocator thread_safe_arena(ArenaAllocator::DEFAULT_BLOCK_SIZE, true);
    
    const int num_threads = 4;
    const int allocations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> thread_ptrs(num_threads);
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&thread_safe_arena, &thread_ptrs, t, allocations_per_thread]() {
            for (int i = 0; i < allocations_per_thread; ++i) {
                void* ptr = thread_safe_arena.allocate(64 + i % 256);
                ASSERT_NE(ptr, nullptr);
                thread_ptrs[t].push_back(ptr);
                
                // Write to allocated memory
                std::memset(ptr, t * 16 + i % 16, 64);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all allocations succeeded
    size_t total_ptrs = 0;
    for (const auto& ptrs : thread_ptrs) {
        total_ptrs += ptrs.size();
    }
    EXPECT_EQ(total_ptrs, num_threads * allocations_per_thread);
    
    // Verify no pointer collisions
    std::set<void*> unique_ptrs;
    for (const auto& ptrs : thread_ptrs) {
        for (void* ptr : ptrs) {
            EXPECT_TRUE(unique_ptrs.insert(ptr).second);
        }
    }
}

// Test scoped arena
TEST_F(ArenaAllocatorTest, ScopedArena) {
    size_t initial_allocated = arena_->get_allocated_size();
    
    {
        ScopedArena scope(*arena_);
        
        // Allocate within scope
        scope.get().allocate(1024);
        scope.get().allocate(2048);
        
        EXPECT_GT(arena_->get_allocated_size(), initial_allocated);
    }
    
    // Arena should be reset after scope
    EXPECT_EQ(arena_->get_allocated_size(), 0u);
}

// Test STL adapter
TEST_F(ArenaAllocatorTest, STLAdapter) {
    using ArenaVector = std::vector<int, ArenaAllocatorAdapter<int>>;
    
    ArenaAllocatorAdapter<int> adapter(*arena_);
    ArenaVector vec(adapter);
    
    // Add elements
    for (int i = 0; i < 1000; ++i) {
        vec.push_back(i * i);
    }
    
    // Verify elements
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(vec[i], i * i);
    }
    
    // Check that arena was used
    EXPECT_GT(arena_->get_allocated_size(), 0u);
}

// Test move semantics
TEST_F(ArenaAllocatorTest, MoveSemantics) {
    arena_->allocate(1024);
    arena_->allocate(2048);
    
    size_t allocated = arena_->get_allocated_size();
    size_t capacity = arena_->get_capacity();
    
    // Move constructor
    ArenaAllocator moved_arena(std::move(*arena_));
    EXPECT_EQ(moved_arena.get_allocated_size(), allocated);
    EXPECT_EQ(moved_arena.get_capacity(), capacity);
    EXPECT_EQ(arena_->get_allocated_size(), 0u);
    EXPECT_EQ(arena_->get_capacity(), 0u);
    
    // Move assignment
    *arena_ = std::move(moved_arena);
    EXPECT_EQ(arena_->get_allocated_size(), allocated);
    EXPECT_EQ(arena_->get_capacity(), capacity);
    EXPECT_EQ(moved_arena.get_allocated_size(), 0u);
    EXPECT_EQ(moved_arena.get_capacity(), 0u);
}

// Performance benchmark
TEST_F(ArenaAllocatorTest, PerformanceComparison) {
    const int num_allocations = 100000;
    const int min_size = 16;
    const int max_size = 256;
    
    // Benchmark arena allocator
    auto arena_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_allocations; ++i) {
        size_t size = min_size + (i % (max_size - min_size));
        void* ptr = arena_->allocate(size);
        ASSERT_NE(ptr, nullptr);
        std::memset(ptr, i & 0xFF, size);
    }
    auto arena_end = std::chrono::high_resolution_clock::now();
    auto arena_duration = std::chrono::duration_cast<std::chrono::microseconds>(arena_end - arena_start);
    
    // Benchmark malloc
    std::vector<void*> malloc_ptrs;
    malloc_ptrs.reserve(num_allocations);
    
    auto malloc_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_allocations; ++i) {
        size_t size = min_size + (i % (max_size - min_size));
        void* ptr = std::malloc(size);
        ASSERT_NE(ptr, nullptr);
        malloc_ptrs.push_back(ptr);
        std::memset(ptr, i & 0xFF, size);
    }
    auto malloc_end = std::chrono::high_resolution_clock::now();
    auto malloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(malloc_end - malloc_start);
    
    // Free malloc allocations
    for (void* ptr : malloc_ptrs) {
        std::free(ptr);
    }
    
    std::cout << "Arena allocator: " << arena_duration.count() << " μs for " 
              << num_allocations << " allocations\n";
    std::cout << "Malloc: " << malloc_duration.count() << " μs for " 
              << num_allocations << " allocations\n";
    std::cout << "Speedup: " << static_cast<double>(malloc_duration.count()) / arena_duration.count() 
              << "x\n";
    
    // Arena should be significantly faster
    EXPECT_LT(arena_duration.count(), malloc_duration.count());
}

// Test zero-size allocation
TEST_F(ArenaAllocatorTest, ZeroSizeAllocation) {
    void* ptr = arena_->allocate(0);
    EXPECT_EQ(ptr, nullptr);
}

// Test allocation pattern
TEST_F(ArenaAllocatorTest, AllocationPattern) {
    // Allocate with specific pattern
    std::vector<void*> ptrs;
    
    // Small allocations
    for (int i = 0; i < 100; ++i) {
        ptrs.push_back(arena_->allocate(8));
    }
    
    // Medium allocations
    for (int i = 0; i < 50; ++i) {
        ptrs.push_back(arena_->allocate(256));
    }
    
    // Large allocations
    for (int i = 0; i < 10; ++i) {
        ptrs.push_back(arena_->allocate(4096));
    }
    
    // All allocations should succeed
    for (void* ptr : ptrs) {
        EXPECT_NE(ptr, nullptr);
    }
    
    // Check utilization
    auto stats = arena_->get_stats();
    EXPECT_GT(stats.utilization_percent, 50.0);
}