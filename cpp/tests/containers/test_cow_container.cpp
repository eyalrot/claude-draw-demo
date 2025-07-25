#include <gtest/gtest.h>
#include "claude_draw/containers/cow_container.h"
#include <thread>
#include <chrono>
#include <vector>

using namespace claude_draw::containers;
using namespace claude_draw::shapes;
using namespace claude_draw;

class CoWContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<CoWContainer>();
    }
    
    std::unique_ptr<CoWContainer> container_;
};

// Test basic CoW semantics
TEST_F(CoWContainerTest, BasicCopyOnWrite) {
    // Add some shapes
    Circle c1(10.0f, 20.0f, 5.0f);
    Circle c2(30.0f, 40.0f, 8.0f);
    uint32_t id1 = container_->add_circle(c1);
    uint32_t id2 = container_->add_circle(c2);
    
    // Create a copy - should be O(1)
    CoWContainer copy = *container_;
    
    // Both should share data
    EXPECT_TRUE(container_->is_shared());
    EXPECT_TRUE(copy.is_shared());
    EXPECT_EQ(container_->ref_count(), 2u);
    EXPECT_EQ(copy.ref_count(), 2u);
    
    // Read operations shouldn't trigger copy
    EXPECT_EQ(container_->size(), 2u);
    EXPECT_EQ(copy.size(), 2u);
    EXPECT_TRUE(container_->contains(id1));
    EXPECT_TRUE(copy.contains(id1));
    
    // Still sharing
    EXPECT_TRUE(container_->is_shared());
    EXPECT_TRUE(copy.is_shared());
    
    // Write operation should trigger copy
    Circle c3(50.0f, 60.0f, 10.0f);
    uint32_t id3 = copy.add_circle(c3);
    
    // Now they should not share data
    EXPECT_FALSE(container_->is_shared());
    EXPECT_FALSE(copy.is_shared());
    EXPECT_EQ(container_->ref_count(), 1u);
    EXPECT_EQ(copy.ref_count(), 1u);
    
    // Original should have 2 shapes, copy should have 3
    EXPECT_EQ(container_->size(), 2u);
    EXPECT_EQ(copy.size(), 3u);
    EXPECT_FALSE(container_->contains(id3));
    EXPECT_TRUE(copy.contains(id3));
}

// Test multiple copies
TEST_F(CoWContainerTest, MultipleCopies) {
    // Add shapes
    for (int i = 0; i < 10; ++i) {
        Circle c(i * 10.0f, i * 20.0f, 5.0f);
        container_->add_circle(c);
    }
    
    // Create multiple copies
    CoWContainer copy1 = *container_;
    CoWContainer copy2 = *container_;
    CoWContainer copy3 = copy1;
    
    // All should share the same data
    EXPECT_EQ(container_->ref_count(), 4u);
    EXPECT_EQ(copy1.ref_count(), 4u);
    EXPECT_EQ(copy2.ref_count(), 4u);
    EXPECT_EQ(copy3.ref_count(), 4u);
    
    // Modify copy1
    copy1.add_circle(Circle(100.0f, 100.0f, 10.0f));
    
    // copy1 should be unique, others still shared
    EXPECT_EQ(copy1.ref_count(), 1u);
    EXPECT_EQ(container_->ref_count(), 3u);
    EXPECT_EQ(copy2.ref_count(), 3u);
    EXPECT_EQ(copy3.ref_count(), 3u);
    
    // Sizes should reflect the change
    EXPECT_EQ(container_->size(), 10u);
    EXPECT_EQ(copy1.size(), 11u);
    EXPECT_EQ(copy2.size(), 10u);
    EXPECT_EQ(copy3.size(), 10u);
}

// Test move semantics
TEST_F(CoWContainerTest, MoveSemantics) {
    // Add shapes
    container_->add_circle(Circle(10.0f, 20.0f, 5.0f));
    container_->add_rectangle(Rectangle(0.0f, 0.0f, 100.0f, 50.0f));
    
    size_t original_size = container_->size();
    
    // Move construct
    CoWContainer moved(std::move(*container_));
    EXPECT_EQ(moved.size(), original_size);
    EXPECT_EQ(container_->size(), 0u);  // Original should be empty
    
    // Move assign
    *container_ = std::move(moved);
    EXPECT_EQ(container_->size(), original_size);
    EXPECT_EQ(moved.size(), 0u);  // Moved-from should be empty
}

// Test forced uniqueness
TEST_F(CoWContainerTest, ForceUnique) {
    // Add shapes
    container_->add_circle(Circle(10.0f, 20.0f, 5.0f));
    
    // Create copies
    CoWContainer copy1 = *container_;
    CoWContainer copy2 = *container_;
    
    EXPECT_EQ(container_->ref_count(), 3u);
    
    // Force unique on copy1
    copy1.make_unique();
    
    // copy1 should be unique
    EXPECT_EQ(copy1.ref_count(), 1u);
    EXPECT_EQ(container_->ref_count(), 2u);
    EXPECT_EQ(copy2.ref_count(), 2u);
}

// Test thread safety
TEST_F(CoWContainerTest, ThreadSafety) {
    // Add initial shapes
    for (int i = 0; i < 100; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        container_->add_circle(c);
    }
    
    // Create multiple copies
    std::vector<CoWContainer> copies;
    for (int i = 0; i < 4; ++i) {
        copies.push_back(*container_);
    }
    
    // Concurrent reads should be safe
    std::vector<std::thread> threads;
    std::atomic<int> total_count(0);
    
    for (auto& copy : copies) {
        threads.emplace_back([&copy, &total_count]() {
            for (int i = 0; i < 100; ++i) {
                size_t count = copy.size();
                total_count.fetch_add(static_cast<int>(count));
                
                // Read bounds
                BoundingBox bounds = copy.get_bounds();
                (void)bounds;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All reads should have seen 100 shapes
    EXPECT_EQ(total_count.load(), 40000);  // 4 threads * 100 iterations * 100 shapes
}

// Test version control
TEST_F(CoWContainerTest, VersionControl) {
    CoWVersionControl vc(10);  // Max 10 versions
    
    // Initial state
    EXPECT_EQ(vc.version_count(), 1u);
    EXPECT_EQ(vc.current_version_index(), 0u);
    
    // Add shapes to version 0
    auto& v0 = vc.current_mutable();
    v0.add_circle(Circle(10.0f, 20.0f, 5.0f));
    v0.add_circle(Circle(30.0f, 40.0f, 8.0f));
    
    // Checkpoint creates version 1
    vc.checkpoint();
    EXPECT_EQ(vc.version_count(), 2u);
    EXPECT_EQ(vc.current_version_index(), 1u);
    
    // Modify version 1
    auto& v1 = vc.current_mutable();
    v1.add_rectangle(Rectangle(0.0f, 0.0f, 100.0f, 50.0f));
    
    // Checkpoint creates version 2
    vc.checkpoint();
    EXPECT_EQ(vc.version_count(), 3u);
    EXPECT_EQ(vc.current_version_index(), 2u);
    
    // Current should have 3 shapes
    EXPECT_EQ(vc.current().size(), 3u);
    
    // Undo to version 1
    EXPECT_TRUE(vc.undo());
    EXPECT_EQ(vc.current_version_index(), 1u);
    EXPECT_EQ(vc.current().size(), 3u);
    
    // Undo to version 0
    EXPECT_TRUE(vc.undo());
    EXPECT_EQ(vc.current_version_index(), 0u);
    EXPECT_EQ(vc.current().size(), 2u);
    
    // Can't undo further
    EXPECT_FALSE(vc.undo());
    
    // Redo
    EXPECT_TRUE(vc.redo());
    EXPECT_EQ(vc.current_version_index(), 1u);
    EXPECT_EQ(vc.current().size(), 3u);
}

// Test version branching
TEST_F(CoWContainerTest, VersionBranching) {
    CoWVersionControl vc;
    
    // Create version history: v0 -> v1 -> v2
    auto& v0 = vc.current_mutable();
    v0.add_circle(Circle(10.0f, 20.0f, 5.0f));
    vc.checkpoint();
    
    auto& v1 = vc.current_mutable();
    v1.add_circle(Circle(30.0f, 40.0f, 8.0f));
    vc.checkpoint();
    
    auto& v2 = vc.current_mutable();
    v2.add_circle(Circle(50.0f, 60.0f, 10.0f));
    
    EXPECT_EQ(vc.version_count(), 3u);
    EXPECT_EQ(vc.current().size(), 3u);
    
    // Go back to v1
    vc.go_to_version(1);
    EXPECT_EQ(vc.current().size(), 2u);
    
    // Make a change - should branch, removing v2
    auto& v1_branch = vc.current_mutable();
    v1_branch.add_rectangle(Rectangle(0.0f, 0.0f, 100.0f, 100.0f));
    
    // After going back and modifying, the future versions should be removed
    EXPECT_EQ(vc.version_count(), 2u);  // v0 and v1 (v2 was removed)
    EXPECT_EQ(vc.current_version_index(), 1u);  // Still at v1
    EXPECT_EQ(vc.current().size(), 3u);  // 2 circles + 1 rectangle
}

// Test max versions limit
TEST_F(CoWContainerTest, MaxVersionsLimit) {
    CoWVersionControl vc(5);  // Max 5 versions
    
    // Create 10 versions
    for (int i = 0; i < 10; ++i) {
        auto& current = vc.current_mutable();
        current.add_circle(Circle(i * 10.0f, i * 20.0f, 5.0f));
        if (i < 9) {
            vc.checkpoint();
        }
    }
    
    // Should only have 5 versions (oldest ones removed)
    EXPECT_EQ(vc.version_count(), 5u);
    EXPECT_EQ(vc.current().size(), 10u);  // Last version has 10 shapes
    
    // Go to oldest available version
    vc.go_to_version(0);
    EXPECT_EQ(vc.current().size(), 6u);  // Version 5 (0-based) had 6 shapes
}

// Test performance comparison
TEST_F(CoWContainerTest, PerformanceComparison) {
    const size_t num_shapes = 10000;
    
    // Add many shapes
    for (size_t i = 0; i < num_shapes; ++i) {
        Circle c(i * 0.1f, i * 0.2f, 1.0f);
        container_->add_circle(c);
    }
    
    // Time copy operation
    auto start = std::chrono::high_resolution_clock::now();
    CoWContainer copy1 = *container_;
    CoWContainer copy2 = *container_;
    CoWContainer copy3 = *container_;
    auto end = std::chrono::high_resolution_clock::now();
    auto copy_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "CoW copy time for " << num_shapes << " shapes: " 
              << copy_time.count() << " μs (3 copies)\n";
    
    // All should share data
    EXPECT_EQ(container_->ref_count(), 4u);
    
    // Time first write (triggers actual copy)
    start = std::chrono::high_resolution_clock::now();
    copy1.add_circle(Circle(999.0f, 999.0f, 10.0f));
    end = std::chrono::high_resolution_clock::now();
    auto cow_trigger_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "CoW trigger time (first write): " << cow_trigger_time.count() << " μs\n";
    
    // Subsequent writes should be fast
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        copy1.add_circle(Circle(i * 1.0f, i * 1.0f, 1.0f));
    }
    end = std::chrono::high_resolution_clock::now();
    auto subsequent_writes = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Subsequent writes (100 shapes): " << subsequent_writes.count() << " μs\n";
}

// Test memory efficiency
TEST_F(CoWContainerTest, MemoryEfficiency) {
    // Create a container with many shapes
    const size_t base_shapes = 1000;
    for (size_t i = 0; i < base_shapes; ++i) {
        Circle c(i * 1.0f, i * 2.0f, 3.0f);
        Rectangle r(i * 2.0f, i * 3.0f, 10.0f, 20.0f);
        container_->add_circle(c);
        container_->add_rectangle(r);
    }
    
    // Create many copies that share data
    std::vector<CoWContainer> copies;
    for (int i = 0; i < 100; ++i) {
        copies.push_back(*container_);
    }
    
    // All should share the same underlying data
    EXPECT_EQ(container_->ref_count(), 101u);
    
    // Modify only a few
    for (int i = 0; i < 5; ++i) {
        copies[i].add_circle(Circle(9999.0f, 9999.0f, 100.0f));
    }
    
    // Check reference counts
    EXPECT_EQ(container_->ref_count(), 96u);  // 101 - 5 modified
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(copies[i].ref_count(), 1u);  // Modified ones are unique
    }
    for (int i = 5; i < 100; ++i) {
        EXPECT_EQ(copies[i].ref_count(), 96u);  // Rest still share
    }
}

// Test clear history
TEST_F(CoWContainerTest, ClearHistory) {
    CoWVersionControl vc;
    
    // Create some history
    for (int i = 0; i < 5; ++i) {
        auto& current = vc.current_mutable();
        current.add_circle(Circle(i * 10.0f, i * 20.0f, 5.0f));
        if (i < 4) {
            vc.checkpoint();
        }
    }
    
    EXPECT_EQ(vc.version_count(), 5u);
    EXPECT_EQ(vc.current_version_index(), 4u);
    EXPECT_EQ(vc.current().size(), 5u);
    
    // Go to middle version
    vc.go_to_version(2);
    EXPECT_EQ(vc.current().size(), 3u);
    
    // Clear history
    vc.clear_history();
    
    // Should only have current version
    EXPECT_EQ(vc.version_count(), 1u);
    EXPECT_EQ(vc.current_version_index(), 0u);
    EXPECT_EQ(vc.current().size(), 3u);  // Kept the version we were on
}