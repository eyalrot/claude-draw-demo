#pragma once

#include <vector>
#include <stack>
#include <memory>
#include <atomic>
#include <mutex>
#include <cstddef>
#include <new>
#include <type_traits>

namespace claude_draw {
namespace memory {

/**
 * @brief A fast, thread-safe object pool for frequent allocations
 * 
 * This pool pre-allocates memory in blocks and reuses objects to avoid
 * the overhead of frequent malloc/free calls. Objects are constructed
 * in-place and can be reset instead of destroyed.
 * 
 * @tparam T The type of object to pool
 * @tparam BlockSize Number of objects per memory block
 */
template<typename T, size_t BlockSize = 1024>
class ObjectPool {
public:
    static_assert(BlockSize > 0, "Block size must be positive");
    
    /**
     * @brief Construct an object pool
     * @param initial_blocks Number of blocks to pre-allocate
     * @param max_blocks Maximum number of blocks (0 = unlimited)
     */
    explicit ObjectPool(size_t initial_blocks = 1, size_t max_blocks = 0)
        : max_blocks_(max_blocks)
        , allocated_count_(0)
        , free_count_(0)
    {
        // Pre-allocate initial blocks
        for (size_t i = 0; i < initial_blocks; ++i) {
            allocate_block();
        }
    }
    
    ~ObjectPool() {
        // Destroy all objects and free memory
        clear();
    }
    
    // Non-copyable, non-movable (for simplicity)
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;
    
    /**
     * @brief Acquire an object from the pool
     * @param args Constructor arguments for T
     * @return Pointer to the acquired object
     */
    template<typename... Args>
    T* acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Try to get from free list
        if (!free_list_.empty()) {
            T* obj = free_list_.top();
            free_list_.pop();
            --free_count_;
            
            // Construct in-place
            new (obj) T(std::forward<Args>(args)...);
            return obj;
        }
        
        // Allocate new object
        if (current_block_ >= current_block_size_) {
            if (!allocate_block()) {
                throw std::bad_alloc();
            }
        }
        
        T* obj = reinterpret_cast<T*>(&(*blocks_.back())[current_block_++]);
        new (obj) T(std::forward<Args>(args)...);
        ++allocated_count_;
        return obj;
    }
    
    /**
     * @brief Release an object back to the pool
     * @param obj Object to release
     */
    void release(T* obj) {
        if (!obj) return;
        
        // Call destructor
        obj->~T();
        
        std::lock_guard<std::mutex> lock(mutex_);
        free_list_.push(obj);
        ++free_count_;
    }
    
    /**
     * @brief Get the number of allocated objects
     */
    size_t allocated_count() const {
        return allocated_count_.load();
    }
    
    /**
     * @brief Get the number of free objects in the pool
     */
    size_t free_count() const {
        return free_count_.load();
    }
    
    /**
     * @brief Get the total capacity (allocated slots)
     */
    size_t capacity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return blocks_.size() * BlockSize;
    }
    
    /**
     * @brief Clear all objects and optionally free memory
     * @param free_memory If true, deallocate all blocks
     */
    void clear(bool free_memory = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Clear free list
        while (!free_list_.empty()) {
            free_list_.pop();
        }
        
        if (free_memory) {
            blocks_.clear();
            current_block_ = 0;
            current_block_size_ = 0;
        }
        
        allocated_count_ = 0;
        free_count_ = 0;
    }
    
    /**
     * @brief Reserve capacity for a given number of objects
     * @param count Number of objects to reserve space for
     */
    void reserve(size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t current_capacity = blocks_.size() * BlockSize;
        if (current_capacity >= count) return;
        
        size_t needed_blocks = (count + BlockSize - 1) / BlockSize;
        size_t blocks_to_add = needed_blocks - blocks_.size();
        
        for (size_t i = 0; i < blocks_to_add; ++i) {
            if (!allocate_block()) break;
        }
    }

private:
    /**
     * @brief Allocate a new block of objects
     * @return True if successful, false if max blocks reached
     */
    bool allocate_block() {
        if (max_blocks_ > 0 && blocks_.size() >= max_blocks_) {
            return false;
        }
        
        // Allocate aligned memory for the block
        auto block = std::make_unique<AlignedBlock>();
        blocks_.push_back(std::move(block));
        
        // Reset current block pointer
        current_block_ = 0;
        current_block_size_ = BlockSize;
        
        return true;
    }
    
    // Aligned storage for objects
    using AlignedStorage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    using AlignedBlock = std::array<AlignedStorage, BlockSize>;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Memory blocks
    std::vector<std::unique_ptr<AlignedBlock>> blocks_;
    size_t current_block_ = 0;
    size_t current_block_size_ = 0;
    
    // Free list
    std::stack<T*> free_list_;
    
    // Configuration
    size_t max_blocks_;
    
    // Statistics
    std::atomic<size_t> allocated_count_;
    std::atomic<size_t> free_count_;
};

/**
 * @brief RAII wrapper for pooled objects
 * 
 * Automatically returns object to pool when destroyed
 */
template<typename T, size_t BlockSize = 1024>
class PooledPtr {
public:
    PooledPtr() = default;
    
    PooledPtr(T* obj, ObjectPool<T, BlockSize>* pool)
        : obj_(obj), pool_(pool) {}
    
    ~PooledPtr() {
        reset();
    }
    
    // Move semantics
    PooledPtr(PooledPtr&& other) noexcept
        : obj_(other.obj_), pool_(other.pool_) {
        other.obj_ = nullptr;
        other.pool_ = nullptr;
    }
    
    PooledPtr& operator=(PooledPtr&& other) noexcept {
        if (this != &other) {
            reset();
            obj_ = other.obj_;
            pool_ = other.pool_;
            other.obj_ = nullptr;
            other.pool_ = nullptr;
        }
        return *this;
    }
    
    // No copy
    PooledPtr(const PooledPtr&) = delete;
    PooledPtr& operator=(const PooledPtr&) = delete;
    
    // Pointer-like interface
    T* get() const { return obj_; }
    T* operator->() const { return obj_; }
    T& operator*() const { return *obj_; }
    explicit operator bool() const { return obj_ != nullptr; }
    
    // Release ownership without returning to pool
    T* release() {
        T* tmp = obj_;
        obj_ = nullptr;
        pool_ = nullptr;
        return tmp;
    }
    
    // Return to pool and reset
    void reset() {
        if (obj_ && pool_) {
            pool_->release(obj_);
        }
        obj_ = nullptr;
        pool_ = nullptr;
    }

private:
    T* obj_ = nullptr;
    ObjectPool<T, BlockSize>* pool_ = nullptr;
};

/**
 * @brief Factory function to acquire from pool with RAII
 */
template<typename T, size_t BlockSize = 1024, typename... Args>
PooledPtr<T, BlockSize> make_pooled(ObjectPool<T, BlockSize>& pool, Args&&... args) {
    T* obj = pool.acquire(std::forward<Args>(args)...);
    return PooledPtr<T, BlockSize>(obj, &pool);
}

} // namespace memory
} // namespace claude_draw