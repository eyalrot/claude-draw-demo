#include "claude_draw/memory/memory_profiler.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace claude_draw {
namespace memory {

// Forward declaration
static std::string format_bytes(size_t bytes);

void MemoryProfiler::track_allocation(void* ptr, size_t size, size_t alignment, 
                                     const std::string& tag) {
    if (!enabled_.load() || !ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t allocation_id = next_allocation_id_.fetch_add(1);
    auto timestamp = std::chrono::steady_clock::now();
    
    // Record allocation
    AllocationInfo info{
        size,
        alignment,
        ptr,
        timestamp,
        tag,
        allocation_id
    };
    
    active_allocations_[ptr] = info;
    
    // Update statistics
    stats_.current_usage += size;
    stats_.total_allocated += size;
    stats_.allocation_count++;
    stats_.active_allocations++;
    
    // Update size histogram
    stats_.size_histogram[size]++;
    
    // Update tag statistics
    stats_.tag_usage[tag] += size;
    stats_.tag_allocations[tag]++;
    
    // Update peak usage
    update_peak_usage();
    
    // Record timeline event
    if (record_timeline_.load()) {
        timeline_.push_back({
            AllocationEvent::Allocate,
            timestamp,
            ptr,
            size,
            tag,
            allocation_id
        });
    }
}

void MemoryProfiler::track_deallocation(void* ptr) {
    if (!enabled_.load() || !ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_allocations_.find(ptr);
    if (it == active_allocations_.end()) {
        // Unknown deallocation - could be from before profiling started
        return;
    }
    
    const auto& info = it->second;
    size_t size = info.size;
    std::string tag = info.tag;
    uint64_t allocation_id = info.allocation_id;
    
    // Update statistics
    stats_.current_usage -= size;
    stats_.total_deallocated += size;
    stats_.deallocation_count++;
    stats_.active_allocations--;
    
    // Update tag statistics
    stats_.tag_usage[tag] -= size;
    
    // Remove from active allocations
    active_allocations_.erase(it);
    
    // Record timeline event
    if (record_timeline_.load()) {
        timeline_.push_back({
            AllocationEvent::Deallocate,
            std::chrono::steady_clock::now(),
            ptr,
            size,
            tag,
            allocation_id
        });
    }
}

MemoryStats MemoryProfiler::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

std::vector<AllocationInfo> MemoryProfiler::get_active_allocations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<AllocationInfo> result;
    result.reserve(active_allocations_.size());
    
    for (const auto& [ptr, info] : active_allocations_) {
        result.push_back(info);
    }
    
    // Sort by allocation time
    std::sort(result.begin(), result.end(), 
              [](const auto& a, const auto& b) {
                  return a.timestamp < b.timestamp;
              });
    
    return result;
}

std::vector<AllocationEvent> MemoryProfiler::get_timeline() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeline_;
}

void MemoryProfiler::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    active_allocations_.clear();
    stats_ = MemoryStats{};
    timeline_.clear();
}

void MemoryProfiler::update_peak_usage() {
    if (stats_.current_usage > stats_.peak_usage) {
        stats_.peak_usage = stats_.current_usage;
    }
}

std::string MemoryProfiler::generate_report() const {
    // Copy data under lock, then work with copies
    MemoryStats stats_copy;
    std::unordered_map<void*, AllocationInfo> active_copy;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_copy = stats_;
        active_copy = active_allocations_;
    }
    
    std::ostringstream oss;
    oss << "=== Memory Profile Report ===\n\n";
    
    // Overall statistics
    oss << "Overall Statistics:\n";
    oss << "  Current usage: " << format_bytes(stats_copy.current_usage) << "\n";
    oss << "  Peak usage: " << format_bytes(stats_copy.peak_usage) << "\n";
    oss << "  Total allocated: " << format_bytes(stats_copy.total_allocated) << "\n";
    oss << "  Total deallocated: " << format_bytes(stats_copy.total_deallocated) << "\n";
    oss << "  Allocations: " << stats_copy.allocation_count << "\n";
    oss << "  Deallocations: " << stats_copy.deallocation_count << "\n";
    oss << "  Active allocations: " << stats_copy.active_allocations << "\n\n";
    
    // Tag-based statistics
    if (!stats_copy.tag_usage.empty()) {
        oss << "Tag-based Usage:\n";
        
        // Sort tags by usage
        std::vector<std::pair<std::string, size_t>> tag_usage(
            stats_copy.tag_usage.begin(), stats_copy.tag_usage.end());
        std::sort(tag_usage.begin(), tag_usage.end(),
                  [](const auto& a, const auto& b) {
                      return a.second > b.second;
                  });
        
        for (const auto& [tag, usage] : tag_usage) {
            auto alloc_it = stats_copy.tag_allocations.find(tag);
            size_t count = (alloc_it != stats_copy.tag_allocations.end()) ? 
                          alloc_it->second : 0;
            
            oss << "  " << std::setw(20) << std::left << tag 
                << ": " << std::setw(10) << std::right << format_bytes(usage)
                << " (" << count << " allocations)\n";
        }
        oss << "\n";
    }
    
    // Size distribution
    if (!stats_copy.size_histogram.empty()) {
        oss << "Size Distribution:\n";
        
        // Sort sizes
        std::vector<std::pair<size_t, size_t>> sizes(
            stats_copy.size_histogram.begin(), stats_copy.size_histogram.end());
        std::sort(sizes.begin(), sizes.end());
        
        for (const auto& [size, count] : sizes) {
            oss << "  " << std::setw(10) << std::right << format_bytes(size)
                << ": " << count << " allocations\n";
        }
        oss << "\n";
    }
    
    // Memory leaks - work with the copy to avoid deadlock
    std::vector<LeakInfo> leaks;
    auto threshold = std::chrono::steady_clock::now() - std::chrono::minutes(1);
    
    for (const auto& [ptr, info] : active_copy) {
        if (info.timestamp < threshold) {
            leaks.push_back({
                info.address,
                info.size,
                info.tag,
                info.timestamp,
                info.allocation_id
            });
        }
    }
    
    // Sort by size (largest leaks first)
    std::sort(leaks.begin(), leaks.end(),
              [](const auto& a, const auto& b) {
                  return a.size > b.size;
              });
    if (!leaks.empty()) {
        oss << "Potential Memory Leaks (" << leaks.size() << " found):\n";
        
        size_t total_leaked = 0;
        for (const auto& leak : leaks) {
            total_leaked += leak.size;
            
            auto duration = std::chrono::duration_cast<std::chrono::seconds>
                           (std::chrono::steady_clock::now() - leak.allocation_time);
            
            oss << "  Address: " << leak.address
                << ", Size: " << format_bytes(leak.size)
                << ", Tag: " << leak.tag
                << ", Age: " << duration.count() << "s"
                << ", ID: " << leak.allocation_id << "\n";
        }
        oss << "  Total leaked: " << format_bytes(total_leaked) << "\n";
    }
    
    return oss.str();
}

std::vector<MemoryProfiler::LeakInfo> MemoryProfiler::detect_leaks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<LeakInfo> leaks;
    
    // Consider allocations older than 1 minute as potential leaks
    auto threshold = std::chrono::steady_clock::now() - std::chrono::minutes(1);
    
    for (const auto& [ptr, info] : active_allocations_) {
        if (info.timestamp < threshold) {
            leaks.push_back({
                info.address,
                info.size,
                info.tag,
                info.timestamp,
                info.allocation_id
            });
        }
    }
    
    // Sort by size (largest leaks first)
    std::sort(leaks.begin(), leaks.end(),
              [](const auto& a, const auto& b) {
                  return a.size > b.size;
              });
    
    return leaks;
}

// Helper function to format bytes
static std::string format_bytes(size_t bytes) {
    std::ostringstream oss;
    
    if (bytes >= 1024 * 1024 * 1024) {
        oss << std::fixed << std::setprecision(2) 
            << static_cast<double>(bytes) / (1024 * 1024 * 1024) << " GB";
    } else if (bytes >= 1024 * 1024) {
        oss << std::fixed << std::setprecision(2) 
            << static_cast<double>(bytes) / (1024 * 1024) << " MB";
    } else if (bytes >= 1024) {
        oss << std::fixed << std::setprecision(2) 
            << static_cast<double>(bytes) / 1024 << " KB";
    } else {
        oss << bytes << " B";
    }
    
    return oss.str();
}

} // namespace memory
} // namespace claude_draw