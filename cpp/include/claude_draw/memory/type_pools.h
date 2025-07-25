#pragma once

#include "claude_draw/memory/object_pool.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"

namespace claude_draw {
namespace memory {

/**
 * @brief Global pools for frequently allocated types
 * 
 * These pools are optimized for the specific allocation patterns
 * of each type. They use different block sizes based on typical usage.
 */
class TypePools {
public:
    // Block sizes optimized for each type
    static constexpr size_t POINT_BLOCK_SIZE = 4096;      // Points are very small and numerous
    static constexpr size_t COLOR_BLOCK_SIZE = 2048;      // Colors are small and frequent
    static constexpr size_t TRANSFORM_BLOCK_SIZE = 256;   // Transforms are larger, less frequent
    static constexpr size_t BBOX_BLOCK_SIZE = 1024;       // BoundingBoxes are medium frequency
    
    // Get singleton instance
    static TypePools& instance() {
        static TypePools pools;
        return pools;
    }
    
    // Pool accessors
    ObjectPool<Point2D, POINT_BLOCK_SIZE>& point_pool() { return point_pool_; }
    ObjectPool<Color, COLOR_BLOCK_SIZE>& color_pool() { return color_pool_; }
    ObjectPool<Transform2D, TRANSFORM_BLOCK_SIZE>& transform_pool() { return transform_pool_; }
    ObjectPool<BoundingBox, BBOX_BLOCK_SIZE>& bbox_pool() { return bbox_pool_; }
    
    // Convenience allocation functions
    template<typename... Args>
    static PooledPtr<Point2D, POINT_BLOCK_SIZE> make_point(Args&&... args) {
        return make_pooled(instance().point_pool_, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static PooledPtr<Color, COLOR_BLOCK_SIZE> make_color(Args&&... args) {
        return make_pooled(instance().color_pool_, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static PooledPtr<Transform2D, TRANSFORM_BLOCK_SIZE> make_transform(Args&&... args) {
        return make_pooled(instance().transform_pool_, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static PooledPtr<BoundingBox, BBOX_BLOCK_SIZE> make_bbox(Args&&... args) {
        return make_pooled(instance().bbox_pool_, std::forward<Args>(args)...);
    }
    
    // Batch allocation support
    std::vector<Point2D*> acquire_points(size_t count) {
        std::vector<Point2D*> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.push_back(point_pool_.acquire());
        }
        
        return result;
    }
    
    void release_points(const std::vector<Point2D*>& points) {
        for (auto* p : points) {
            point_pool_.release(p);
        }
    }
    
    // Pre-allocate memory
    void reserve_points(size_t count) {
        point_pool_.reserve(count);
    }
    
    void reserve_colors(size_t count) {
        color_pool_.reserve(count);
    }
    
    void reserve_transforms(size_t count) {
        transform_pool_.reserve(count);
    }
    
    void reserve_bboxes(size_t count) {
        bbox_pool_.reserve(count);
    }
    
    // Statistics
    struct PoolStats {
        size_t allocated;
        size_t free;
        size_t capacity;
    };
    
    PoolStats point_stats() const {
        return {
            point_pool_.allocated_count(),
            point_pool_.free_count(),
            point_pool_.capacity()
        };
    }
    
    PoolStats color_stats() const {
        return {
            color_pool_.allocated_count(),
            color_pool_.free_count(),
            color_pool_.capacity()
        };
    }
    
    PoolStats transform_stats() const {
        return {
            transform_pool_.allocated_count(),
            transform_pool_.free_count(),
            transform_pool_.capacity()
        };
    }
    
    PoolStats bbox_stats() const {
        return {
            bbox_pool_.allocated_count(),
            bbox_pool_.free_count(),
            bbox_pool_.capacity()
        };
    }
    
    // Clear all pools
    void clear_all(bool free_memory = false) {
        point_pool_.clear(free_memory);
        color_pool_.clear(free_memory);
        transform_pool_.clear(free_memory);
        bbox_pool_.clear(free_memory);
    }

private:
    TypePools() 
        : point_pool_(4)      // Pre-allocate 4 blocks of points
        , color_pool_(2)      // Pre-allocate 2 blocks of colors
        , transform_pool_(1)  // Pre-allocate 1 block of transforms
        , bbox_pool_(1)       // Pre-allocate 1 block of bboxes
    {}
    
    ObjectPool<Point2D, POINT_BLOCK_SIZE> point_pool_;
    ObjectPool<Color, COLOR_BLOCK_SIZE> color_pool_;
    ObjectPool<Transform2D, TRANSFORM_BLOCK_SIZE> transform_pool_;
    ObjectPool<BoundingBox, BBOX_BLOCK_SIZE> bbox_pool_;
};

/**
 * @brief Batch allocator for creating many objects at once
 */
template<typename T>
class BatchAllocator {
public:
    explicit BatchAllocator(size_t reserve_count = 1000) {
        objects_.reserve(reserve_count);
    }
    
    ~BatchAllocator() {
        clear();
    }
    
    // Allocate a single object
    template<typename... Args>
    T* allocate(Args&&... args) {
        objects_.emplace_back(std::forward<Args>(args)...);
        return &objects_.back();
    }
    
    // Allocate multiple objects with same parameters
    template<typename... Args>
    std::vector<T*> allocate_many(size_t count, Args&&... args) {
        std::vector<T*> result;
        result.reserve(count);
        
        size_t start_idx = objects_.size();
        objects_.reserve(start_idx + count);
        
        for (size_t i = 0; i < count; ++i) {
            objects_.emplace_back(std::forward<Args>(args)...);
            result.push_back(&objects_.back());
        }
        
        return result;
    }
    
    // Get all allocated objects
    std::vector<T*> get_all() {
        std::vector<T*> result;
        result.reserve(objects_.size());
        
        for (auto& obj : objects_) {
            result.push_back(&obj);
        }
        
        return result;
    }
    
    // Clear all objects
    void clear() {
        objects_.clear();
    }
    
    // Get count
    size_t size() const {
        return objects_.size();
    }

private:
    std::vector<T> objects_;
};

} // namespace memory
} // namespace claude_draw