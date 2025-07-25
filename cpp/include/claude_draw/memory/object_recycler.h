#pragma once

#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <mutex>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <cstddef>  // for offsetof

namespace claude_draw {
namespace memory {

/**
 * @brief High-performance object recycler with size-class buckets
 * 
 * Provides efficient object recycling by maintaining free lists for
 * different size classes. This avoids the overhead of frequent
 * allocations and deallocations.
 * 
 * Features:
 * - O(1) allocation and deallocation
 * - Size-class bucketing to reduce fragmentation
 * - Thread-safe operation with minimal contention
 * - Automatic memory reclamation
 * - Statistics tracking
 */
template<typename T>
class ObjectRecycler {
public:
    static constexpr size_t DEFAULT_MAX_FREE_PER_SIZE = 1000;
    static constexpr size_t DEFAULT_MAX_TOTAL_FREE = 10000;
    
    explicit ObjectRecycler(bool thread_safe = false,
                           size_t max_free_per_size = DEFAULT_MAX_FREE_PER_SIZE,
                           size_t max_total_free = DEFAULT_MAX_TOTAL_FREE)
        : thread_safe_(thread_safe),
          max_free_per_size_(max_free_per_size),
          max_total_free_(max_total_free),
          total_free_(0),
          total_allocated_(0),
          total_recycled_(0) {}
    
    ~ObjectRecycler() {
        clear();
    }
    
    // Disable copy
    ObjectRecycler(const ObjectRecycler&) = delete;
    ObjectRecycler& operator=(const ObjectRecycler&) = delete;
    
    // Enable move
    ObjectRecycler(ObjectRecycler&& other) noexcept = default;
    ObjectRecycler& operator=(ObjectRecycler&& other) noexcept = default;
    
    /**
     * @brief Allocate an object, using recycled memory if available
     */
    template<typename... Args>
    T* allocate(Args&&... args) {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            return allocate_impl(std::forward<Args>(args)...);
        }
        return allocate_impl(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Deallocate an object, adding it to the free list for recycling
     */
    void deallocate(T* obj) {
        if (!obj) return;
        
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            deallocate_impl(obj);
        } else {
            deallocate_impl(obj);
        }
    }
    
    /**
     * @brief Clear all free lists, releasing memory
     */
    void clear() {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            clear_impl();
        } else {
            clear_impl();
        }
    }
    
    /**
     * @brief Trim free lists to maximum sizes
     */
    void trim() {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            trim_impl();
        } else {
            trim_impl();
        }
    }
    
    /**
     * @brief Get recycling statistics
     */
    struct Stats {
        size_t total_allocated;
        size_t total_recycled;
        size_t current_free;
        size_t max_free_per_size;
        size_t max_total_free;
        double recycle_rate;
    };
    
    Stats get_stats() const {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            return get_stats_impl();
        }
        return get_stats_impl();
    }
    
    /**
     * @brief Get number of free objects
     */
    size_t get_free_count() const {
        return total_free_.load();
    }
    
private:
    struct FreeNode {
        alignas(T) char storage[sizeof(T)];
    };
    
    template<typename... Args>
    T* allocate_impl(Args&&... args) {
        // Try to get from free list
        if (!free_list_.empty()) {
            FreeNode* node = free_list_.back();
            free_list_.pop_back();
            total_free_--;
            total_recycled_++;
            
            // Construct object in recycled memory
            return new (node->storage) T(std::forward<Args>(args)...);
        }
        
        // Allocate new object as FreeNode to ensure proper size
        total_allocated_++;
        FreeNode* node = static_cast<FreeNode*>(::operator new(sizeof(FreeNode)));
        return new (node->storage) T(std::forward<Args>(args)...);
    }
    
    void deallocate_impl(T* obj) {
        // Destruct object
        obj->~T();
        
        // The object was allocated inside FreeNode::storage, which is at offset 0
        FreeNode* node = reinterpret_cast<FreeNode*>(obj);
        
        // Check if we should add to free list
        if (total_free_ < max_total_free_ && free_list_.size() < max_free_per_size_) {
            free_list_.push_back(node);
            total_free_++;
        } else {
            // Actually free the memory (use operator delete to avoid calling destructor again)
            ::operator delete(node);
        }
    }
    
    void clear_impl() {
        for (FreeNode* node : free_list_) {
            ::operator delete(node);
        }
        free_list_.clear();
        total_free_ = 0;
    }
    
    void trim_impl() {
        while (free_list_.size() > max_free_per_size_) {
            FreeNode* node = free_list_.back();
            free_list_.pop_back();
            ::operator delete(node);
            total_free_--;
        }
    }
    
    Stats get_stats_impl() const {
        size_t allocated = total_allocated_.load();
        size_t recycled = total_recycled_.load();
        size_t total_used = allocated + recycled;
        
        return Stats{
            allocated,
            recycled,
            total_free_.load(),
            max_free_per_size_,
            max_total_free_,
            total_used > 0 ? (100.0 * recycled / total_used) : 0.0
        };
    }
    
    std::vector<FreeNode*> free_list_;
    bool thread_safe_;
    size_t max_free_per_size_;
    size_t max_total_free_;
    std::atomic<size_t> total_free_;
    std::atomic<size_t> total_allocated_;
    std::atomic<size_t> total_recycled_;
    mutable std::mutex mutex_;
};

/**
 * @brief Multi-size object recycler with size classes
 * 
 * Manages recycling for objects of different sizes using size-class
 * buckets. This is more efficient than having separate recyclers
 * for each exact size.
 */
class MultiSizeRecycler {
public:
    static constexpr size_t NUM_SIZE_CLASSES = 32;
    static constexpr size_t MIN_SIZE = 8;
    static constexpr size_t MAX_SIZE = 65536;
    
    explicit MultiSizeRecycler(bool thread_safe = false)
        : thread_safe_(thread_safe),
          total_allocated_(0),
          total_recycled_(0) {
        initialize_size_classes();
    }
    
    ~MultiSizeRecycler() {
        clear();
    }
    
    // Disable copy
    MultiSizeRecycler(const MultiSizeRecycler&) = delete;
    MultiSizeRecycler& operator=(const MultiSizeRecycler&) = delete;
    
    /**
     * @brief Allocate memory of given size
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        if (size == 0) return nullptr;
        
        size_t aligned_size = align_size(size, alignment);
        size_t class_idx = get_size_class(aligned_size);
        
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutexes_[class_idx]);
            return allocate_from_class(class_idx, aligned_size);
        }
        return allocate_from_class(class_idx, aligned_size);
    }
    
    /**
     * @brief Deallocate memory
     */
    void deallocate(void* ptr, size_t size) {
        if (!ptr || size == 0) return;
        
        size_t aligned_size = align_size(size, alignof(std::max_align_t));
        
        // For allocations larger than our max size class, just free directly
        if (aligned_size > MAX_SIZE) {
            std::free(ptr);
            return;
        }
        
        size_t class_idx = get_size_class(aligned_size);
        
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutexes_[class_idx]);
            deallocate_to_class(ptr, class_idx);
        } else {
            deallocate_to_class(ptr, class_idx);
        }
    }
    
    /**
     * @brief Clear all free lists
     */
    void clear() {
        for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
            if (thread_safe_) {
                std::lock_guard<std::mutex> lock(mutexes_[i]);
                clear_class(i);
            } else {
                clear_class(i);
            }
        }
    }
    
    /**
     * @brief Get statistics
     */
    struct Stats {
        size_t total_allocated;
        size_t total_recycled;
        size_t total_free;
        std::array<size_t, NUM_SIZE_CLASSES> free_per_class;
        double recycle_rate;
    };
    
    Stats get_stats() const {
        Stats stats;
        stats.total_allocated = total_allocated_.load();
        stats.total_recycled = total_recycled_.load();
        stats.total_free = 0;
        
        for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
            if (thread_safe_) {
                std::lock_guard<std::mutex> lock(mutexes_[i]);
                stats.free_per_class[i] = free_lists_[i].size();
            } else {
                stats.free_per_class[i] = free_lists_[i].size();
            }
            stats.total_free += stats.free_per_class[i];
        }
        
        size_t total = stats.total_allocated + stats.total_recycled;
        stats.recycle_rate = total > 0 ? (100.0 * stats.total_recycled / total) : 0.0;
        
        return stats;
    }
    
private:
    struct FreeNode {
        FreeNode* next;
    };
    
    void initialize_size_classes() {
        size_t size = MIN_SIZE;
        for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
            size_classes_[i] = size;
            if (i < NUM_SIZE_CLASSES - 1) {
                size = std::min(size * 2, MAX_SIZE);
            }
        }
    }
    
    size_t get_size_class(size_t size) const {
        // Binary search for appropriate size class
        auto it = std::lower_bound(size_classes_.begin(), size_classes_.end(), size);
        if (it == size_classes_.end()) {
            return NUM_SIZE_CLASSES - 1;
        }
        return std::distance(size_classes_.begin(), it);
    }
    
    size_t align_size(size_t size, size_t alignment) const {
        return (size + alignment - 1) & ~(alignment - 1);
    }
    
    void* allocate_from_class(size_t class_idx, size_t requested_size) {
        size_t alloc_size = size_classes_[class_idx];
        
        // For allocations larger than our max size class, allocate the exact size
        if (requested_size > alloc_size) {
            void* ptr = std::aligned_alloc(alignof(std::max_align_t), requested_size);
            if (!ptr) {
                throw std::bad_alloc();
            }
            total_allocated_++;
            return ptr;
        }
        
        auto& free_list = free_lists_[class_idx];
        
        if (!free_list.empty()) {
            void* ptr = free_list.back();
            free_list.pop_back();
            total_recycled_++;
            return ptr;
        }
        
        // Allocate new block
        void* ptr = std::aligned_alloc(alignof(std::max_align_t), alloc_size);
        if (!ptr) {
            throw std::bad_alloc();
        }
        total_allocated_++;
        return ptr;
    }
    
    void deallocate_to_class(void* ptr, size_t class_idx) {
        auto& free_list = free_lists_[class_idx];
        
        // Limit free list size
        if (free_list.size() < 1000) {
            free_list.push_back(ptr);
        } else {
            std::free(ptr);
        }
    }
    
    void clear_class(size_t class_idx) {
        auto& free_list = free_lists_[class_idx];
        for (void* ptr : free_list) {
            std::free(ptr);
        }
        free_list.clear();
    }
    
    std::array<size_t, NUM_SIZE_CLASSES> size_classes_;
    std::array<std::vector<void*>, NUM_SIZE_CLASSES> free_lists_;
    bool thread_safe_;
    mutable std::array<std::mutex, NUM_SIZE_CLASSES> mutexes_;
    std::atomic<size_t> total_allocated_;
    std::atomic<size_t> total_recycled_;
};

} // namespace memory
} // namespace claude_draw