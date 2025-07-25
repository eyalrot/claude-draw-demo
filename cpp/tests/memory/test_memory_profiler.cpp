#include <gtest/gtest.h>
#include "claude_draw/memory/memory_profiler.h"
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace claude_draw::memory;

class MemoryProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear profiler and enable it
        MemoryProfiler::instance().clear();
        MemoryProfiler::instance().set_enabled(true);
        MemoryProfiler::instance().set_record_timeline(true);
    }
    
    void TearDown() override {
        // Disable profiler
        MemoryProfiler::instance().set_enabled(false);
        MemoryProfiler::instance().clear();
    }
};

// Test basic tracking
TEST_F(MemoryProfilerTest, BasicTracking) {
    auto& profiler = MemoryProfiler::instance();
    
    // Allocate some memory
    void* ptr1 = malloc(1024);
    profiler.track_allocation(ptr1, 1024, 1, "test1");
    
    void* ptr2 = malloc(2048);
    profiler.track_allocation(ptr2, 2048, 1, "test2");
    
    // Check statistics
    auto stats = profiler.get_stats();
    EXPECT_EQ(stats.current_usage, 3072u);
    EXPECT_EQ(stats.peak_usage, 3072u);
    EXPECT_EQ(stats.allocation_count, 2u);
    EXPECT_EQ(stats.active_allocations, 2u);
    
    // Deallocate
    profiler.track_deallocation(ptr1);
    free(ptr1);
    
    stats = profiler.get_stats();
    EXPECT_EQ(stats.current_usage, 2048u);
    EXPECT_EQ(stats.peak_usage, 3072u);
    EXPECT_EQ(stats.deallocation_count, 1u);
    EXPECT_EQ(stats.active_allocations, 1u);
    
    profiler.track_deallocation(ptr2);
    free(ptr2);
    
    stats = profiler.get_stats();
    EXPECT_EQ(stats.current_usage, 0u);
    EXPECT_EQ(stats.active_allocations, 0u);
}

// Test tag-based tracking
TEST_F(MemoryProfilerTest, TagBasedTracking) {
    auto& profiler = MemoryProfiler::instance();
    
    // Allocate with different tags
    void* ptr1 = malloc(1024);
    profiler.track_allocation(ptr1, 1024, 1, "shapes");
    
    void* ptr2 = malloc(2048);
    profiler.track_allocation(ptr2, 2048, 1, "containers");
    
    void* ptr3 = malloc(512);
    profiler.track_allocation(ptr3, 512, 1, "shapes");
    
    auto stats = profiler.get_stats();
    EXPECT_EQ(stats.tag_usage["shapes"], 1536u);
    EXPECT_EQ(stats.tag_usage["containers"], 2048u);
    EXPECT_EQ(stats.tag_allocations["shapes"], 2u);
    EXPECT_EQ(stats.tag_allocations["containers"], 1u);
    
    // Cleanup
    profiler.track_deallocation(ptr1);
    profiler.track_deallocation(ptr2);
    profiler.track_deallocation(ptr3);
    free(ptr1);
    free(ptr2);
    free(ptr3);
}

// Test size histogram
TEST_F(MemoryProfilerTest, SizeHistogram) {
    auto& profiler = MemoryProfiler::instance();
    
    std::vector<void*> ptrs;
    
    // Allocate various sizes
    for (size_t size : {64, 128, 256, 64, 128, 64}) {
        void* ptr = malloc(size);
        ptrs.push_back(ptr);
        profiler.track_allocation(ptr, size, 1, "test");
    }
    
    auto stats = profiler.get_stats();
    EXPECT_EQ(stats.size_histogram[64], 3u);
    EXPECT_EQ(stats.size_histogram[128], 2u);
    EXPECT_EQ(stats.size_histogram[256], 1u);
    
    // Cleanup
    for (void* ptr : ptrs) {
        profiler.track_deallocation(ptr);
        free(ptr);
    }
}

// Test timeline recording
TEST_F(MemoryProfilerTest, TimelineRecording) {
    auto& profiler = MemoryProfiler::instance();
    
    // Make some allocations
    void* ptr1 = malloc(1024);
    profiler.track_allocation(ptr1, 1024, 1, "test");
    
    void* ptr2 = malloc(2048);
    profiler.track_allocation(ptr2, 2048, 1, "test");
    
    profiler.track_deallocation(ptr1);
    free(ptr1);
    
    auto timeline = profiler.get_timeline();
    ASSERT_EQ(timeline.size(), 3u);
    
    EXPECT_EQ(timeline[0].type, AllocationEvent::Allocate);
    EXPECT_EQ(timeline[0].size, 1024u);
    
    EXPECT_EQ(timeline[1].type, AllocationEvent::Allocate);
    EXPECT_EQ(timeline[1].size, 2048u);
    
    EXPECT_EQ(timeline[2].type, AllocationEvent::Deallocate);
    EXPECT_EQ(timeline[2].size, 1024u);
    
    // Cleanup
    profiler.track_deallocation(ptr2);
    free(ptr2);
}

// Test profiler scope
TEST_F(MemoryProfilerTest, ProfilerScope) {
    auto& profiler = MemoryProfiler::instance();
    
    auto initial_stats = profiler.get_stats();
    
    {
        ProfilerScope scope("test_operation");
        
        void* ptr = malloc(4096);
        profiler.track_allocation(ptr, 4096, 1, "scoped");
        
        // Scope destructor will capture stats
        profiler.track_deallocation(ptr);
        free(ptr);
    }
    
    // Stats should show the allocation happened
    auto final_stats = profiler.get_stats();
    EXPECT_EQ(final_stats.total_allocated - initial_stats.total_allocated, 4096u);
    EXPECT_EQ(final_stats.total_deallocated - initial_stats.total_deallocated, 4096u);
}

// Test tracked allocator
TEST_F(MemoryProfilerTest, TrackedAllocator) {
    auto& profiler = MemoryProfiler::instance();
    
    // Use tracked allocator with vector
    std::vector<int, TrackedAllocator<int>> vec;
    vec.reserve(100);  // Should trigger allocation
    
    auto stats = profiler.get_stats();
    EXPECT_GT(stats.current_usage, 0u);
    EXPECT_EQ(stats.tag_usage["TrackedAllocator"], stats.current_usage);
    
    // Add elements
    for (int i = 0; i < 1000; ++i) {
        vec.push_back(i);  // May trigger reallocation
    }
    
    stats = profiler.get_stats();
    EXPECT_GT(stats.allocation_count, 1u);  // Should have reallocated
}

// Test memory monitor
TEST_F(MemoryProfilerTest, MemoryMonitor) {
    auto& profiler = MemoryProfiler::instance();
    
    MemoryMonitor monitor;
    
    // Allocate some memory
    void* ptr1 = malloc(1024);
    profiler.track_allocation(ptr1, 1024, 1, "monitor_test");
    
    void* ptr2 = malloc(2048);
    profiler.track_allocation(ptr2, 2048, 1, "monitor_test");
    
    auto usage = monitor.get_usage();
    EXPECT_EQ(usage.allocated, 3072u);
    EXPECT_EQ(usage.deallocated, 0u);
    
    // Deallocate one
    profiler.track_deallocation(ptr1);
    free(ptr1);
    
    usage = monitor.get_usage();
    EXPECT_EQ(usage.allocated, 3072u);
    EXPECT_EQ(usage.deallocated, 1024u);
    
    // Cleanup
    profiler.track_deallocation(ptr2);
    free(ptr2);
}

// Test leak detection
TEST_F(MemoryProfilerTest, LeakDetection) {
    auto& profiler = MemoryProfiler::instance();
    
    // Allocate some memory
    void* ptr1 = malloc(1024);
    profiler.track_allocation(ptr1, 1024, 1, "potential_leak");
    
    void* ptr2 = malloc(2048);
    profiler.track_allocation(ptr2, 2048, 1, "normal");
    
    // Immediately check for leaks (nothing should be detected yet)
    auto leaks = profiler.detect_leaks();
    EXPECT_EQ(leaks.size(), 0u);  // Too new to be considered leaks
    
    // Clean up to avoid actual leaks
    profiler.track_deallocation(ptr1);
    profiler.track_deallocation(ptr2);
    free(ptr1);
    free(ptr2);
}

// Test report generation
TEST_F(MemoryProfilerTest, ReportGeneration) {
    auto& profiler = MemoryProfiler::instance();
    
    // Create some allocations
    std::vector<void*> ptrs;
    
    for (size_t i = 0; i < 5; ++i) {
        size_t size = (i + 1) * 256;
        void* ptr = malloc(size);
        ptrs.push_back(ptr);
        profiler.track_allocation(ptr, size, 1, 
                                 i % 2 == 0 ? "even" : "odd");
    }
    
    // Generate report
    std::string report = profiler.generate_report();
    
    // Check report contains expected sections
    EXPECT_NE(report.find("Memory Profile Report"), std::string::npos);
    EXPECT_NE(report.find("Overall Statistics"), std::string::npos);
    EXPECT_NE(report.find("Tag-based Usage"), std::string::npos);
    EXPECT_NE(report.find("Size Distribution"), std::string::npos);
    
    // Cleanup
    for (void* ptr : ptrs) {
        profiler.track_deallocation(ptr);
        free(ptr);
    }
}

// Test thread safety
TEST_F(MemoryProfilerTest, ThreadSafety) {
    auto& profiler = MemoryProfiler::instance();
    
    const int num_threads = 4;
    const int allocations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> thread_allocations(num_threads);
    
    // Spawn threads that allocate and deallocate
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < allocations_per_thread; ++i) {
                size_t size = (t + 1) * 128 + i * 8;
                void* ptr = malloc(size);
                thread_allocations[t].push_back(ptr);
                
                profiler.track_allocation(ptr, size, 1, 
                                        "thread_" + std::to_string(t));
                
                // Random delay
                if (i % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
            
            // Deallocate half
            for (size_t i = 0; i < thread_allocations[t].size() / 2; ++i) {
                void* ptr = thread_allocations[t][i];
                profiler.track_deallocation(ptr);
                free(ptr);
            }
        });
    }
    
    // Wait for threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check statistics
    auto stats = profiler.get_stats();
    EXPECT_EQ(stats.allocation_count, 
              static_cast<size_t>(num_threads * allocations_per_thread));
    EXPECT_EQ(stats.deallocation_count, 
              static_cast<size_t>(num_threads * allocations_per_thread / 2));
    
    // Cleanup remaining allocations
    for (int t = 0; t < num_threads; ++t) {
        for (size_t i = thread_allocations[t].size() / 2; 
             i < thread_allocations[t].size(); ++i) {
            void* ptr = thread_allocations[t][i];
            profiler.track_deallocation(ptr);
            free(ptr);
        }
    }
}

// Test enable/disable functionality
TEST_F(MemoryProfilerTest, EnableDisable) {
    auto& profiler = MemoryProfiler::instance();
    
    // Disable profiler
    profiler.set_enabled(false);
    
    void* ptr = malloc(1024);
    profiler.track_allocation(ptr, 1024, 1, "disabled");
    
    auto stats = profiler.get_stats();
    EXPECT_EQ(stats.current_usage, 0u);  // Should not track
    
    // Enable and track
    profiler.set_enabled(true);
    profiler.track_allocation(ptr, 1024, 1, "enabled");
    
    stats = profiler.get_stats();
    EXPECT_EQ(stats.current_usage, 1024u);  // Should track now
    
    // Cleanup
    profiler.track_deallocation(ptr);
    free(ptr);
}

// Test peak usage tracking
TEST_F(MemoryProfilerTest, PeakUsageTracking) {
    auto& profiler = MemoryProfiler::instance();
    
    // Allocate and deallocate in waves
    for (int wave = 0; wave < 3; ++wave) {
        std::vector<void*> ptrs;
        
        // Allocate increasing amounts
        for (int i = 0; i < 5; ++i) {
            size_t size = (wave + 1) * 1024;
            void* ptr = malloc(size);
            ptrs.push_back(ptr);
            profiler.track_allocation(ptr, size, 1, "wave");
        }
        
        // Check peak
        auto stats = profiler.get_stats();
        size_t expected_usage = (wave + 1) * 1024 * 5;
        EXPECT_EQ(stats.current_usage, expected_usage);
        EXPECT_GE(stats.peak_usage, expected_usage);
        
        // Deallocate all
        for (void* ptr : ptrs) {
            profiler.track_deallocation(ptr);
            free(ptr);
        }
    }
    
    // Final peak should be from the largest wave
    auto final_stats = profiler.get_stats();
    EXPECT_EQ(final_stats.current_usage, 0u);
    EXPECT_EQ(final_stats.peak_usage, 3 * 1024 * 5);
}

// Test clear functionality
TEST_F(MemoryProfilerTest, ClearProfiling) {
    auto& profiler = MemoryProfiler::instance();
    
    // Make some allocations
    void* ptr1 = malloc(1024);
    profiler.track_allocation(ptr1, 1024, 1, "test");
    
    void* ptr2 = malloc(2048);
    profiler.track_allocation(ptr2, 2048, 1, "test");
    
    // Clear profiler
    profiler.clear();
    
    // Check everything is reset
    auto stats = profiler.get_stats();
    EXPECT_EQ(stats.current_usage, 0u);
    EXPECT_EQ(stats.peak_usage, 0u);
    EXPECT_EQ(stats.allocation_count, 0u);
    EXPECT_EQ(stats.active_allocations, 0u);
    EXPECT_TRUE(stats.tag_usage.empty());
    EXPECT_TRUE(stats.size_histogram.empty());
    
    auto timeline = profiler.get_timeline();
    EXPECT_TRUE(timeline.empty());
    
    auto active = profiler.get_active_allocations();
    EXPECT_TRUE(active.empty());
    
    // Cleanup actual memory
    free(ptr1);
    free(ptr2);
}