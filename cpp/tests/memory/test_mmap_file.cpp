#include <gtest/gtest.h>
#include "claude_draw/memory/mmap_file.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>
#include <cstring>

using namespace claude_draw::memory;
namespace fs = std::filesystem;

class MemoryMappedFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        temp_dir_ = fs::temp_directory_path() / "mmap_test";
        fs::create_directories(temp_dir_);
    }
    
    void TearDown() override {
        // Clean up test files
        fs::remove_all(temp_dir_);
    }
    
    fs::path get_test_file(const std::string& name) {
        return temp_dir_ / name;
    }
    
    void create_test_file(const fs::path& path, size_t size, char fill = 'A') {
        std::ofstream file(path, std::ios::binary);
        std::vector<char> data(size, fill);
        file.write(data.data(), size);
    }
    
    fs::path temp_dir_;
};

// Test basic file operations
TEST_F(MemoryMappedFileTest, BasicFileOperations) {
    auto test_file = get_test_file("basic.dat");
    
    MemoryMappedFile mmap;
    
    // Test creating new file
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 1024));
    EXPECT_TRUE(mmap.is_open());
    EXPECT_EQ(mmap.size(), 1024u);
    EXPECT_NE(mmap.data(), nullptr);
    
    // Write data
    char* data = static_cast<char*>(mmap.data());
    std::strcpy(data, "Hello, Memory Mapped World!");
    
    // Close and reopen
    mmap.close();
    EXPECT_FALSE(mmap.is_open());
    EXPECT_EQ(mmap.data(), nullptr);
    
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadOnly));
    EXPECT_EQ(mmap.size(), 1024u);
    
    // Verify data persisted
    const char* read_data = static_cast<const char*>(mmap.data());
    EXPECT_STREQ(read_data, "Hello, Memory Mapped World!");
}

// Test file resizing
TEST_F(MemoryMappedFileTest, FileResizing) {
    auto test_file = get_test_file("resize.dat");
    
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 1024));
    
    // Write initial data
    std::memset(mmap.data(), 'A', 1024);
    
    // Resize larger
    ASSERT_TRUE(mmap.resize(2048));
    EXPECT_EQ(mmap.size(), 2048u);
    
    // Verify original data preserved
    char* data = static_cast<char*>(mmap.data());
    for (size_t i = 0; i < 1024; ++i) {
        EXPECT_EQ(data[i], 'A');
    }
    
    // Write to new region
    std::memset(data + 1024, 'B', 1024);
    
    // Resize smaller
    ASSERT_TRUE(mmap.resize(512));
    EXPECT_EQ(mmap.size(), 512u);
    
    // Verify data still accessible
    for (size_t i = 0; i < 512; ++i) {
        EXPECT_EQ(data[i], 'A');
    }
}

// Test read-only access
TEST_F(MemoryMappedFileTest, ReadOnlyAccess) {
    auto test_file = get_test_file("readonly.dat");
    create_test_file(test_file, 256, 'X');
    
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadOnly));
    
    // Verify data
    const char* data = static_cast<const char*>(mmap.data());
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(data[i], 'X');
    }
    
    // Cannot resize read-only file
    EXPECT_FALSE(mmap.resize(512));
}

// Test sync operations
TEST_F(MemoryMappedFileTest, SyncOperations) {
    auto test_file = get_test_file("sync.dat");
    
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 4096));
    
    // Write data
    std::strcpy(static_cast<char*>(mmap.data()), "Sync test data");
    
    // Sync to disk
    EXPECT_TRUE(mmap.sync(MemoryMappedFile::SyncMode::Sync));
    
    // Verify file size on disk
    EXPECT_EQ(fs::file_size(test_file), 4096u);
}

// Test move semantics
TEST_F(MemoryMappedFileTest, MoveSemantics) {
    auto test_file = get_test_file("move.dat");
    
    MemoryMappedFile mmap1;
    ASSERT_TRUE(mmap1.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 1024));
    std::strcpy(static_cast<char*>(mmap1.data()), "Move test");
    
    // Move constructor
    MemoryMappedFile mmap2(std::move(mmap1));
    EXPECT_FALSE(mmap1.is_open());
    EXPECT_TRUE(mmap2.is_open());
    EXPECT_EQ(mmap2.size(), 1024u);
    EXPECT_STREQ(static_cast<char*>(mmap2.data()), "Move test");
    
    // Move assignment
    MemoryMappedFile mmap3;
    mmap3 = std::move(mmap2);
    EXPECT_FALSE(mmap2.is_open());
    EXPECT_TRUE(mmap3.is_open());
    EXPECT_STREQ(static_cast<char*>(mmap3.data()), "Move test");
}

// Test typed access
TEST_F(MemoryMappedFileTest, TypedAccess) {
    auto test_file = get_test_file("typed.dat");
    
    struct TestStruct {
        int32_t id;
        double value;
        char name[32];
    };
    
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 
                          sizeof(TestStruct) * 10));
    
    // Access as typed array
    TestStruct* structs = mmap.data_as<TestStruct>();
    for (int i = 0; i < 10; ++i) {
        structs[i].id = i;
        structs[i].value = i * 3.14;
        std::snprintf(structs[i].name, sizeof(structs[i].name), "Item_%d", i);
    }
    
    // Use span interface
    auto span = mmap.as_span<TestStruct>();
    EXPECT_EQ(span.size(), 10u);
    
    for (size_t i = 0; i < span.size(); ++i) {
        EXPECT_EQ(span[i].id, static_cast<int32_t>(i));
        EXPECT_DOUBLE_EQ(span[i].value, i * 3.14);
    }
}

// Test empty file handling
TEST_F(MemoryMappedFileTest, EmptyFile) {
    auto test_file = get_test_file("empty.dat");
    create_test_file(test_file, 0);
    
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite));
    EXPECT_EQ(mmap.size(), 0u);
    EXPECT_EQ(mmap.data(), nullptr);  // Empty files aren't mapped
    
    // Resize to non-zero
    ASSERT_TRUE(mmap.resize(1024));
    EXPECT_EQ(mmap.size(), 1024u);
    EXPECT_NE(mmap.data(), nullptr);
}

// Test memory-mapped arena
TEST_F(MemoryMappedFileTest, MemoryMappedArena) {
    auto arena_file = get_test_file("arena.dat");
    
    MemoryMappedArena arena;
    ASSERT_TRUE(arena.open(arena_file.string(), true));
    
    // Allocate various sizes
    void* ptr1 = arena.allocate(64);
    ASSERT_NE(ptr1, nullptr);
    std::memset(ptr1, 'A', 64);
    
    void* ptr2 = arena.allocate(128, 16);  // 16-byte alignment
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 16, 0u);
    std::memset(ptr2, 'B', 128);
    
    void* ptr3 = arena.allocate(256);
    ASSERT_NE(ptr3, nullptr);
    std::memset(ptr3, 'C', 256);
    
    // Check statistics
    EXPECT_GE(arena.get_used_size(), 64u + 128u + 256u);
    EXPECT_GT(arena.get_total_size(), arena.get_used_size());
    
    // Sync and close
    EXPECT_TRUE(arena.sync());
    arena.close();
    
    // Reopen and verify data
    ASSERT_TRUE(arena.open(arena_file.string(), false));
    EXPECT_TRUE(arena.validate());
    
    // Data should be preserved
    char* data = static_cast<char*>(ptr1);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(data[i], 'A');
    }
}

// Test arena growth
TEST_F(MemoryMappedFileTest, ArenaGrowth) {
    auto arena_file = get_test_file("arena_grow.dat");
    
    MemoryMappedArena arena(1024);  // Small block size for testing
    ASSERT_TRUE(arena.open(arena_file.string(), true));
    
    size_t initial_total = arena.get_total_size();
    size_t initial_available = arena.get_available_size();
    
    // Allocate more than available space to force growth
    // Initial size is 4096, minus header leaves ~3992 available
    void* large_alloc = arena.allocate(initial_available + 100);
    ASSERT_NE(large_alloc, nullptr);
    
    // Arena should have grown
    EXPECT_GT(arena.get_total_size(), initial_total);
    EXPECT_GE(arena.get_total_size(), initial_available + 100);
}

// Test arena reset
TEST_F(MemoryMappedFileTest, ArenaReset) {
    auto arena_file = get_test_file("arena_reset.dat");
    
    MemoryMappedArena arena;
    ASSERT_TRUE(arena.open(arena_file.string(), true));
    
    // Allocate some memory
    arena.allocate(512);
    arena.allocate(1024);
    
    size_t used_before = arena.get_used_size();
    EXPECT_GT(used_before, 0u);
    
    // Reset arena
    arena.reset();
    EXPECT_EQ(arena.get_used_size(), 0u);
    
    // Can allocate again
    void* ptr = arena.allocate(256);
    ASSERT_NE(ptr, nullptr);
}

// Test memory-mapped vector
TEST_F(MemoryMappedFileTest, MemoryMappedVector) {
    auto vector_file = get_test_file("vector.dat");
    
    MemoryMappedVector<int32_t> vec;
    ASSERT_TRUE(vec.open(vector_file.string(), true));
    
    // Vector operations
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);
    
    // Push elements
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i * i);
    }
    
    EXPECT_EQ(vec.size(), 100u);
    EXPECT_FALSE(vec.empty());
    
    // Access elements
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(vec[i], i * i);
        EXPECT_EQ(vec.at(i), i * i);
    }
    
    // Iterator access
    int expected = 0;
    for (int val : vec) {
        EXPECT_EQ(val, expected * expected);
        expected++;
    }
    
    // Modify elements
    vec[50] = -1;
    EXPECT_EQ(vec[50], -1);
    
    vec.close();
    
    // Reopen and verify persistence
    ASSERT_TRUE(vec.open(vector_file.string(), false));
    EXPECT_EQ(vec.size(), 100u);
    EXPECT_EQ(vec[50], -1);
    
    for (int i = 0; i < 100; ++i) {
        if (i != 50) {
            EXPECT_EQ(vec[i], i * i);
        }
    }
}

// Test vector growth
TEST_F(MemoryMappedFileTest, VectorGrowth) {
    auto vector_file = get_test_file("vector_grow.dat");
    
    MemoryMappedVector<double> vec;
    ASSERT_TRUE(vec.open(vector_file.string(), true));
    
    size_t initial_capacity = vec.capacity();
    
    // Force growth
    for (int i = 0; i < 1000; ++i) {
        vec.push_back(i * 0.5);
    }
    
    EXPECT_GT(vec.capacity(), initial_capacity);
    EXPECT_EQ(vec.size(), 1000u);
    
    // Verify data integrity after growth
    for (int i = 0; i < 1000; ++i) {
        EXPECT_DOUBLE_EQ(vec[i], i * 0.5);
    }
}

// Test vector operations
TEST_F(MemoryMappedFileTest, VectorOperations) {
    auto vector_file = get_test_file("vector_ops.dat");
    
    struct Point {
        float x, y;
    };
    
    MemoryMappedVector<Point> vec;
    ASSERT_TRUE(vec.open(vector_file.string(), true));
    
    // Reserve capacity
    vec.reserve(50);
    EXPECT_GE(vec.capacity(), 50u);
    
    // Push elements
    for (int i = 0; i < 10; ++i) {
        vec.push_back({static_cast<float>(i), static_cast<float>(i * 2)});
    }
    
    // Access front/back
    EXPECT_FLOAT_EQ(vec.front().x, 0.0f);
    EXPECT_FLOAT_EQ(vec.back().x, 9.0f);
    
    // Pop back
    vec.pop_back();
    EXPECT_EQ(vec.size(), 9u);
    EXPECT_FLOAT_EQ(vec.back().x, 8.0f);
    
    // Clear
    vec.clear();
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);
    
    // Resize
    vec.resize(20);
    EXPECT_EQ(vec.size(), 20u);
}

// Test access patterns
TEST_F(MemoryMappedFileTest, AccessPatterns) {
    auto test_file = get_test_file("access.dat");
    
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 
                          1024 * 1024));  // 1MB
    
    // Advise sequential access
    EXPECT_TRUE(mmap.advise(MemoryMappedFile::AccessPattern::Sequential));
    
    // Write sequentially
    char* data = static_cast<char*>(mmap.data());
    for (size_t i = 0; i < mmap.size(); ++i) {
        data[i] = static_cast<char>(i & 0xFF);
    }
    
    // Advise random access for a region
    EXPECT_TRUE(mmap.advise(MemoryMappedFile::AccessPattern::Random, 0, 4096));
    
    // Advise don't need for unused region
    EXPECT_TRUE(mmap.advise(MemoryMappedFile::AccessPattern::DontNeed, 
                           512 * 1024, 512 * 1024));
}

// Test file locking (may require admin privileges)
TEST_F(MemoryMappedFileTest, FileLocking) {
    auto test_file = get_test_file("lock.dat");
    
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 4096));
    
    // Try to lock pages (may fail without privileges)
    bool lock_result = mmap.lock(0, 4096);
    if (lock_result) {
        // If locking succeeded, test unlock
        EXPECT_TRUE(mmap.unlock(0, 4096));
    }
}

// Test concurrent access (basic thread safety of file operations)
TEST_F(MemoryMappedFileTest, ConcurrentAccess) {
    auto test_file = get_test_file("concurrent.dat");
    
    // Create and initialize file
    {
        MemoryMappedFile mmap;
        ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadWrite, 
                             sizeof(std::atomic<int>) * 10));
        std::memset(mmap.data(), 0, mmap.size());
    }
    
    const int num_threads = 4;
    const int increments_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([test_file, t, increments_per_thread]() {
            MemoryMappedFile mmap;
            ASSERT_TRUE(mmap.open(test_file.string(), 
                                 MemoryMappedFile::Mode::ReadWrite));
            
            std::atomic<int>* counters = mmap.data_as<std::atomic<int>>();
            
            for (int i = 0; i < increments_per_thread; ++i) {
                counters[t].fetch_add(1, std::memory_order_relaxed);
            }
            
            mmap.sync();
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify results
    MemoryMappedFile mmap;
    ASSERT_TRUE(mmap.open(test_file.string(), MemoryMappedFile::Mode::ReadOnly));
    std::atomic<int>* counters = mmap.data_as<std::atomic<int>>();
    
    for (int t = 0; t < num_threads; ++t) {
        EXPECT_EQ(counters[t].load(), increments_per_thread);
    }
}

// Performance test
TEST_F(MemoryMappedFileTest, PerformanceComparison) {
    auto mmap_file = get_test_file("perf_mmap.dat");
    auto regular_file = get_test_file("perf_regular.dat");
    
    const size_t data_size = 10 * 1024 * 1024;  // 10MB
    std::vector<char> test_data(data_size);
    
    // Generate random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto& byte : test_data) {
        byte = static_cast<char>(dis(gen));
    }
    
    // Benchmark memory-mapped write
    auto mmap_start = std::chrono::high_resolution_clock::now();
    {
        MemoryMappedFile mmap;
        ASSERT_TRUE(mmap.open(mmap_file.string(), 
                             MemoryMappedFile::Mode::ReadWrite, data_size));
        std::memcpy(mmap.data(), test_data.data(), data_size);
        mmap.sync();
    }
    auto mmap_end = std::chrono::high_resolution_clock::now();
    
    // Benchmark regular file write
    auto regular_start = std::chrono::high_resolution_clock::now();
    {
        std::ofstream file(regular_file, std::ios::binary);
        file.write(test_data.data(), data_size);
        file.flush();
    }
    auto regular_end = std::chrono::high_resolution_clock::now();
    
    auto mmap_duration = std::chrono::duration_cast<std::chrono::microseconds>
                        (mmap_end - mmap_start);
    auto regular_duration = std::chrono::duration_cast<std::chrono::microseconds>
                           (regular_end - regular_start);
    
    std::cout << "Memory-mapped write: " << mmap_duration.count() << " μs\n";
    std::cout << "Regular file write: " << regular_duration.count() << " μs\n";
    std::cout << "Speedup: " << static_cast<double>(regular_duration.count()) / 
                                mmap_duration.count() << "x\n";
}