#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>

namespace claude_draw {
namespace memory {

/**
 * @brief Memory allocation tracking information
 */
struct AllocationInfo {
    size_t size;
    size_t alignment;
    void* address;
    std::chrono::steady_clock::time_point timestamp;
    std::string tag;
    uint64_t allocation_id;
};

/**
 * @brief Memory usage statistics
 */
struct MemoryStats {
    size_t current_usage = 0;
    size_t peak_usage = 0;
    size_t total_allocated = 0;
    size_t total_deallocated = 0;
    size_t allocation_count = 0;
    size_t deallocation_count = 0;
    size_t active_allocations = 0;
    
    // Size distribution
    std::unordered_map<size_t, size_t> size_histogram;
    
    // Tag-based statistics
    std::unordered_map<std::string, size_t> tag_usage;
    std::unordered_map<std::string, size_t> tag_allocations;
};

/**
 * @brief Memory allocation event for timeline analysis
 */
struct AllocationEvent {
    enum Type { Allocate, Deallocate };
    
    Type type;
    std::chrono::steady_clock::time_point timestamp;
    void* address;
    size_t size;
    std::string tag;
    uint64_t allocation_id;
};

/**
 * @brief Memory profiler for tracking allocations and usage patterns
 */
class MemoryProfiler {
public:
    static MemoryProfiler& instance() {
        static MemoryProfiler profiler;
        return profiler;
    }
    
    /**
     * @brief Track an allocation
     */
    void track_allocation(void* ptr, size_t size, size_t alignment = 1, 
                         const std::string& tag = "");
    
    /**
     * @brief Track a deallocation
     */
    void track_deallocation(void* ptr);
    
    /**
     * @brief Get current memory statistics
     */
    MemoryStats get_stats() const;
    
    /**
     * @brief Get detailed allocation information
     */
    std::vector<AllocationInfo> get_active_allocations() const;
    
    /**
     * @brief Get allocation timeline
     */
    std::vector<AllocationEvent> get_timeline() const;
    
    /**
     * @brief Clear all tracking data
     */
    void clear();
    
    /**
     * @brief Enable/disable profiling
     */
    void set_enabled(bool enabled) { enabled_.store(enabled); }
    bool is_enabled() const { return enabled_.load(); }
    
    /**
     * @brief Set timeline recording
     */
    void set_record_timeline(bool record) { record_timeline_.store(record); }
    bool is_recording_timeline() const { return record_timeline_.load(); }
    
    /**
     * @brief Generate a report
     */
    std::string generate_report() const;
    
    /**
     * @brief Memory leak detection
     */
    struct LeakInfo {
        void* address;
        size_t size;
        std::string tag;
        std::chrono::steady_clock::time_point allocation_time;
        uint64_t allocation_id;
    };
    
    std::vector<LeakInfo> detect_leaks() const;
    
private:
    MemoryProfiler() = default;
    
    mutable std::mutex mutex_;
    std::atomic<bool> enabled_{false};
    std::atomic<bool> record_timeline_{false};
    std::atomic<uint64_t> next_allocation_id_{1};
    
    // Current state
    std::unordered_map<void*, AllocationInfo> active_allocations_;
    MemoryStats stats_;
    
    // Timeline recording
    std::vector<AllocationEvent> timeline_;
    
    // Helper to update peak usage
    void update_peak_usage();
};

/**
 * @brief RAII profiler scope
 */
class ProfilerScope {
public:
    explicit ProfilerScope(const std::string& name)
        : name_(name), start_time_(std::chrono::steady_clock::now()) {
        initial_stats_ = MemoryProfiler::instance().get_stats();
    }
    
    ~ProfilerScope() {
        auto end_time = std::chrono::steady_clock::now();
        auto final_stats = MemoryProfiler::instance().get_stats();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                       (end_time - start_time_).count();
        
        size_t allocated = final_stats.total_allocated - initial_stats_.total_allocated;
        size_t deallocated = final_stats.total_deallocated - initial_stats_.total_deallocated;
        size_t net_change = final_stats.current_usage - initial_stats_.current_usage;
        
        // Could log or store these metrics
        (void)duration;
        (void)allocated;
        (void)deallocated;
        (void)net_change;
    }
    
private:
    std::string name_;
    std::chrono::steady_clock::time_point start_time_;
    MemoryStats initial_stats_;
};

/**
 * @brief Tracked allocator wrapper
 */
template<typename T>
class TrackedAllocator {
public:
    using value_type = T;
    
    TrackedAllocator() = default;
    
    template<typename U>
    TrackedAllocator(const TrackedAllocator<U>&) noexcept {}
    
    T* allocate(size_t n) {
        size_t size = n * sizeof(T);
        void* ptr = std::aligned_alloc(alignof(T), size);
        if (!ptr) {
            throw std::bad_alloc();
        }
        
        MemoryProfiler::instance().track_allocation(ptr, size, alignof(T), 
                                                   "TrackedAllocator");
        return static_cast<T*>(ptr);
    }
    
    void deallocate(T* ptr, size_t) {
        if (ptr) {
            MemoryProfiler::instance().track_deallocation(ptr);
            std::free(ptr);
        }
    }
};

template<typename T, typename U>
bool operator==(const TrackedAllocator<T>&, const TrackedAllocator<U>&) { return true; }

template<typename T, typename U>
bool operator!=(const TrackedAllocator<T>&, const TrackedAllocator<U>&) { return false; }

/**
 * @brief Memory usage monitor for specific operations
 */
class MemoryMonitor {
public:
    MemoryMonitor() : initial_stats_(MemoryProfiler::instance().get_stats()) {}
    
    struct Usage {
        size_t allocated = 0;
        size_t deallocated = 0;
        size_t peak = 0;
        size_t current = 0;
    };
    
    Usage get_usage() const {
        auto current_stats = MemoryProfiler::instance().get_stats();
        Usage usage;
        usage.allocated = current_stats.total_allocated - initial_stats_.total_allocated;
        usage.deallocated = current_stats.total_deallocated - initial_stats_.total_deallocated;
        usage.peak = current_stats.peak_usage;
        usage.current = current_stats.current_usage;
        return usage;
    }
    
private:
    MemoryStats initial_stats_;
};

// Convenience macros
#define PROFILE_MEMORY_SCOPE(name) \
    claude_draw::memory::ProfilerScope _profiler_scope_##__LINE__(name)

#define TRACK_ALLOCATION(ptr, size, tag) \
    claude_draw::memory::MemoryProfiler::instance().track_allocation(ptr, size, 1, tag)

#define TRACK_DEALLOCATION(ptr) \
    claude_draw::memory::MemoryProfiler::instance().track_deallocation(ptr)

} // namespace memory
} // namespace claude_draw