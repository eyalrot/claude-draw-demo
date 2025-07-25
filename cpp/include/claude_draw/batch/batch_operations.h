#pragma once

#include <vector>
#include <span>
#include <algorithm>
#include <execution>

#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"
#include "claude_draw/memory/type_pools.h"

namespace claude_draw {
namespace batch {

/**
 * @brief Batch creation utilities for high-performance object instantiation
 */
class BatchCreator {
public:
    /**
     * @brief Create a batch of points with the same coordinates
     * @param count Number of points to create
     * @param x X coordinate for all points
     * @param y Y coordinate for all points
     * @return Vector of point pointers (owned by caller)
     */
    static std::vector<Point2D*> create_points(size_t count, float x = 0.0f, float y = 0.0f) {
        std::vector<Point2D*> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.push_back(new Point2D(x, y));
        }
        
        return result;
    }
    
    /**
     * @brief Create a batch of points from coordinate arrays
     * @param x_coords Array of X coordinates
     * @param y_coords Array of Y coordinates
     * @param count Number of points to create
     * @return Vector of point pointers (owned by caller)
     */
    static std::vector<Point2D*> create_points(const float* x_coords, const float* y_coords, size_t count) {
        std::vector<Point2D*> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.push_back(new Point2D(x_coords[i], y_coords[i]));
        }
        
        return result;
    }
    
    /**
     * @brief Create a batch of points using object pool
     * @param count Number of points to create
     * @param x X coordinate for all points
     * @param y Y coordinate for all points
     * @return Vector of pooled point pointers
     */
    static std::vector<memory::PooledPtr<Point2D, memory::TypePools::POINT_BLOCK_SIZE>> 
    create_pooled_points(size_t count, float x = 0.0f, float y = 0.0f) {
        auto& pools = memory::TypePools::instance();
        std::vector<memory::PooledPtr<Point2D, memory::TypePools::POINT_BLOCK_SIZE>> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.push_back(memory::TypePools::make_point(x, y));
        }
        
        return result;
    }
    
    /**
     * @brief Create a batch of colors with the same value
     * @param count Number of colors to create
     * @param rgba RGBA value for all colors
     * @return Vector of color pointers (owned by caller)
     */
    static std::vector<Color*> create_colors(size_t count, uint32_t rgba = 0xFF000000) {
        std::vector<Color*> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.push_back(new Color(rgba));
        }
        
        return result;
    }
    
    /**
     * @brief Create a batch of colors from component arrays
     * @param r_values Array of red values
     * @param g_values Array of green values
     * @param b_values Array of blue values
     * @param a_values Array of alpha values (optional)
     * @param count Number of colors to create
     * @return Vector of color pointers (owned by caller)
     */
    static std::vector<Color*> create_colors(
        const uint8_t* r_values, 
        const uint8_t* g_values,
        const uint8_t* b_values,
        const uint8_t* a_values,
        size_t count) {
        
        std::vector<Color*> result;
        result.reserve(count);
        
        if (a_values) {
            for (size_t i = 0; i < count; ++i) {
                result.push_back(new Color(r_values[i], g_values[i], b_values[i], a_values[i]));
            }
        } else {
            for (size_t i = 0; i < count; ++i) {
                result.push_back(new Color(r_values[i], g_values[i], b_values[i]));
            }
        }
        
        return result;
    }
    
    /**
     * @brief Create a batch of identity transforms
     * @param count Number of transforms to create
     * @return Vector of transform pointers (owned by caller)
     */
    static std::vector<Transform2D*> create_transforms(size_t count) {
        std::vector<Transform2D*> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.push_back(new Transform2D());
        }
        
        return result;
    }
    
    /**
     * @brief Create a batch of bounding boxes
     * @param count Number of boxes to create
     * @param min_x Minimum X for all boxes
     * @param min_y Minimum Y for all boxes
     * @param max_x Maximum X for all boxes
     * @param max_y Maximum Y for all boxes
     * @return Vector of bounding box pointers (owned by caller)
     */
    static std::vector<BoundingBox*> create_bounding_boxes(
        size_t count, 
        float min_x = 0.0f, 
        float min_y = 0.0f,
        float max_x = 1.0f,
        float max_y = 1.0f) {
        
        std::vector<BoundingBox*> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.push_back(new BoundingBox(min_x, min_y, max_x, max_y));
        }
        
        return result;
    }
    
    /**
     * @brief Delete a batch of objects
     * @tparam T Type of objects to delete
     * @param objects Vector of object pointers to delete
     */
    template<typename T>
    static void destroy_batch(std::vector<T*>& objects) {
        for (auto* obj : objects) {
            delete obj;
        }
        objects.clear();
    }
};

/**
 * @brief Batch processing utilities for parallel operations
 */
class BatchProcessor {
public:
    /**
     * @brief Apply a transform to a batch of points in parallel
     * @param transform Transform to apply
     * @param points Points to transform (modified in-place)
     */
    static void transform_points(const Transform2D& transform, std::span<Point2D> points) {
        std::for_each(std::execution::par_unseq, points.begin(), points.end(),
            [&transform](Point2D& p) {
                p = transform.transform_point(p);
            });
    }
    
    /**
     * @brief Blend a batch of foreground colors over background colors
     * @param fg_colors Foreground colors
     * @param bg_colors Background colors  
     * @param results Output colors (must be pre-allocated)
     * @param count Number of colors to process
     */
    static void blend_colors(
        const Color* fg_colors,
        const Color* bg_colors,
        Color* results,
        size_t count) {
        
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            results[i] = fg_colors[i].blend_over(bg_colors[i]);
        }
    }
    
    /**
     * @brief Check if points are contained in a bounding box
     * @param box Bounding box to test against
     * @param points Points to test
     * @param results Output array of bools (must be pre-allocated)
     * @param count Number of points to test
     */
    static void contains_points(
        const BoundingBox& box,
        const Point2D* points,
        bool* results,
        size_t count) {
        
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            results[i] = box.contains(points[i]);
        }
    }
    
    /**
     * @brief Calculate distances between pairs of points
     * @param points1 First set of points
     * @param points2 Second set of points
     * @param distances Output array of distances (must be pre-allocated)
     * @param count Number of point pairs
     */
    static void calculate_distances(
        const Point2D* points1,
        const Point2D* points2,
        float* distances,
        size_t count) {
        
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            distances[i] = points1[i].distance_to(points2[i]);
        }
    }
};

/**
 * @brief Memory-efficient batch builder using contiguous allocation
 */
template<typename T>
class ContiguousBatch {
public:
    explicit ContiguousBatch(size_t capacity = 1000) {
        data_.reserve(capacity);
    }
    
    /**
     * @brief Add an object to the batch
     * @param args Constructor arguments for T
     * @return Pointer to the constructed object
     */
    template<typename... Args>
    T* emplace(Args&&... args) {
        data_.emplace_back(std::forward<Args>(args)...);
        return &data_.back();
    }
    
    /**
     * @brief Get all objects as a span
     */
    std::span<T> as_span() {
        return std::span<T>(data_);
    }
    
    /**
     * @brief Get all objects as a const span
     */
    std::span<const T> as_span() const {
        return std::span<const T>(data_);
    }
    
    /**
     * @brief Get the number of objects
     */
    size_t size() const {
        return data_.size();
    }
    
    /**
     * @brief Clear all objects
     */
    void clear() {
        data_.clear();
    }
    
    /**
     * @brief Reserve capacity
     */
    void reserve(size_t capacity) {
        data_.reserve(capacity);
    }
    
    /**
     * @brief Get raw data pointer
     */
    T* data() {
        return data_.data();
    }
    
    /**
     * @brief Get const raw data pointer
     */
    const T* data() const {
        return data_.data();
    }

private:
    std::vector<T> data_;
};

} // namespace batch
} // namespace claude_draw