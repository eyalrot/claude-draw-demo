#pragma once

#include <memory>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <cassert>
#include <limits>

namespace claude_draw {
namespace memory {

/**
 * @brief Arena allocator for fast, bulk allocations
 * 
 * Provides extremely fast allocation by simply incrementing a pointer.
 * Deallocation is not supported for individual objects - instead, the
 * entire arena can be reset or destroyed.
 * 
 * Features:
 * - O(1) allocation
 * - Zero per-allocation overhead
 * - Cache-friendly sequential memory
 * - Thread-safe with optional locking
 * - Automatic growth when needed
 */
class ArenaAllocator {
public:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 64 * 1024;  // 64KB blocks
    static constexpr size_t DEFAULT_ALIGNMENT = 16;          // SSE alignment
    
    explicit ArenaAllocator(size_t block_size = DEFAULT_BLOCK_SIZE,
                           bool thread_safe = false)
        : block_size_(block_size),
          thread_safe_(thread_safe),
          current_block_(0),
          current_offset_(0),
          total_allocated_(0),
          total_capacity_(0) {
        allocate_new_block();
    }
    
    ~ArenaAllocator() {
        for (auto& block : blocks_) {
            std::free(block.data);
        }
    }
    
    // Disable copy
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;
    
    // Enable move
    ArenaAllocator(ArenaAllocator&& other) noexcept
        : blocks_(std::move(other.blocks_)),
          block_size_(other.block_size_),
          thread_safe_(other.thread_safe_),
          current_block_(other.current_block_),
          current_offset_(other.current_offset_),
          total_allocated_(other.total_allocated_.load()),
          total_capacity_(other.total_capacity_.load()) {
        other.current_block_ = 0;
        other.current_offset_ = 0;
        other.total_allocated_ = 0;
        other.total_capacity_ = 0;
    }
    
    ArenaAllocator& operator=(ArenaAllocator&& other) noexcept {
        if (this != &other) {
            // Free existing blocks
            for (auto& block : blocks_) {
                std::free(block.data);
            }
            
            blocks_ = std::move(other.blocks_);
            block_size_ = other.block_size_;
            thread_safe_ = other.thread_safe_;
            current_block_ = other.current_block_;
            current_offset_ = other.current_offset_;
            total_allocated_ = other.total_allocated_.load();
            total_capacity_ = other.total_capacity_.load();
            
            other.current_block_ = 0;
            other.current_offset_ = 0;
            other.total_allocated_ = 0;
            other.total_capacity_ = 0;
        }
        return *this;
    }
    
    /**
     * @brief Allocate memory from the arena
     * @param size Number of bytes to allocate
     * @param alignment Required alignment (must be power of 2)
     * @return Pointer to allocated memory, or nullptr on failure
     */
    void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) {
        assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
        
        if (size == 0) return nullptr;
        
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            return allocate_impl(size, alignment);
        } else {
            return allocate_impl(size, alignment);
        }
    }
    
    /**
     * @brief Allocate and construct an object
     */
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        if (!ptr) return nullptr;
        return new (ptr) T(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Allocate array of objects
     */
    template<typename T>
    T* allocate_array(size_t count) {
        if (count == 0) return nullptr;
        
        size_t size = sizeof(T) * count;
        void* ptr = allocate(size, alignof(T));
        if (!ptr) return nullptr;
        
        return static_cast<T*>(ptr);
    }
    
    /**
     * @brief Reset the arena, making all memory available again
     * Note: Does NOT call destructors on allocated objects
     */
    void reset() {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            reset_impl();
        } else {
            reset_impl();
        }
    }
    
    /**
     * @brief Get total memory allocated
     */
    size_t get_allocated_size() const {
        return total_allocated_.load();
    }
    
    /**
     * @brief Get total memory capacity
     */
    size_t get_capacity() const {
        return total_capacity_.load();
    }
    
    /**
     * @brief Get number of blocks allocated
     */
    size_t get_block_count() const {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            return blocks_.size();
        }
        return blocks_.size();
    }
    
    /**
     * @brief Get memory usage statistics
     */
    struct Stats {
        size_t allocated_bytes;
        size_t capacity_bytes;
        size_t block_count;
        size_t block_size;
        double utilization_percent;
    };
    
    Stats get_stats() const {
        size_t allocated = total_allocated_.load();
        size_t capacity = total_capacity_.load();
        
        return Stats{
            allocated,
            capacity,
            get_block_count(),
            block_size_,
            capacity > 0 ? (100.0 * allocated / capacity) : 0.0
        };
    }
    
private:
    struct Block {
        void* data;
        size_t size;
        
        Block(size_t s) : size(s) {
            data = std::aligned_alloc(DEFAULT_ALIGNMENT, s);
            if (!data) {
                throw std::bad_alloc();
            }
        }
    };
    
    void* allocate_impl(size_t size, size_t alignment) {
        // Align current offset
        size_t aligned_offset = align_up(current_offset_, alignment);
        
        // Check if we need to allocate a new block
        if (aligned_offset + size > block_size_) {
            // If requested size is larger than block size, allocate a custom block
            if (size > block_size_) {
                return allocate_large_block(size, alignment);
            }
            
            // Otherwise, allocate a new standard block
            allocate_new_block();
            aligned_offset = align_up(0, alignment);  // Apply alignment to new block
        }
        
        Block& block = blocks_[current_block_];
        void* ptr = static_cast<char*>(block.data) + aligned_offset;
        
        current_offset_ = aligned_offset + size;
        total_allocated_ += size;
        
        return ptr;
    }
    
    void allocate_new_block() {
        blocks_.emplace_back(block_size_);
        current_block_ = blocks_.size() - 1;
        current_offset_ = 0;
        total_capacity_ += block_size_;
    }
    
    void* allocate_large_block(size_t size, size_t alignment) {
        size_t aligned_size = align_up(size, alignment);
        blocks_.emplace_back(aligned_size);
        total_capacity_ += aligned_size;
        total_allocated_ += size;
        return blocks_.back().data;
    }
    
    void reset_impl() {
        current_block_ = 0;
        current_offset_ = 0;
        total_allocated_ = 0;
        // Keep blocks allocated for reuse
    }
    
    static size_t align_up(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
    
    std::vector<Block> blocks_;
    size_t block_size_;
    bool thread_safe_;
    size_t current_block_;
    size_t current_offset_;
    std::atomic<size_t> total_allocated_;
    std::atomic<size_t> total_capacity_;
    mutable std::mutex mutex_;
};

/**
 * @brief Scoped arena that automatically resets on destruction
 */
class ScopedArena {
public:
    explicit ScopedArena(ArenaAllocator& arena) : arena_(arena) {}
    ~ScopedArena() { arena_.reset(); }
    
    // Disable copy and move
    ScopedArena(const ScopedArena&) = delete;
    ScopedArena& operator=(const ScopedArena&) = delete;
    ScopedArena(ScopedArena&&) = delete;
    ScopedArena& operator=(ScopedArena&&) = delete;
    
    ArenaAllocator& get() { return arena_; }
    
private:
    ArenaAllocator& arena_;
};

/**
 * @brief STL-compatible allocator adapter for ArenaAllocator
 */
template<typename T>
class ArenaAllocatorAdapter {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = ArenaAllocatorAdapter<U>;
    };
    
    ArenaAllocatorAdapter(ArenaAllocator& arena) : arena_(&arena) {}
    
    template<typename U>
    ArenaAllocatorAdapter(const ArenaAllocatorAdapter<U>& other)
        : arena_(other.arena_) {}
    
    T* allocate(size_type n) {
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }
    
    void deallocate(T*, size_type) {
        // No-op for arena allocator
    }
    
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new (p) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
    
    bool operator==(const ArenaAllocatorAdapter& other) const {
        return arena_ == other.arena_;
    }
    
    bool operator!=(const ArenaAllocatorAdapter& other) const {
        return !(*this == other);
    }
    
private:
    ArenaAllocator* arena_;
    
    template<typename U>
    friend class ArenaAllocatorAdapter;
};

} // namespace memory
} // namespace claude_draw