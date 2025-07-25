#pragma once

#include "circle_optimized.h"
#include "rectangle_optimized.h"
#include "ellipse_optimized.h"
#include "line_optimized.h"
#include "../core/simd.h"
#include <vector>
#include <memory>

namespace claude_draw {
namespace shapes {
namespace batch {

/**
 * @brief Unified batch operations for all shape types
 * 
 * This namespace provides high-performance batch operations that work across
 * different shape types. Operations are optimized for SIMD and multi-threading.
 */

// Forward declarations for shape visitors
template<typename Result>
class ShapeBatchVisitor;

/**
 * @brief Batch bounding box calculation for mixed shape types
 */
void calculate_bounds(const void* shapes[], const ShapeType types[], 
                     size_t count, BoundingBox results[]);

/**
 * @brief Batch transformation for mixed shape types
 */
void transform_shapes(void* shapes[], const ShapeType types[], 
                     size_t count, const Transform2D& transform);

/**
 * @brief Batch point containment test for mixed shape types
 */
void contains_points(const void* shapes[], const ShapeType types[],
                    size_t count, const Point2D points[], bool results[]);

/**
 * @brief Calculate union bounds of all shapes
 */
BoundingBox calculate_union_bounds(const void* shapes[], const ShapeType types[], 
                                  size_t count);

/**
 * @brief Filter shapes by visibility flag
 */
size_t filter_visible(const void* shapes[], const ShapeType types[],
                     size_t count, void* visible_shapes[], ShapeType visible_types[]);

/**
 * @brief Batch color operations
 */
void set_fill_colors(void* shapes[], const ShapeType types[],
                    size_t count, const Color& color);

void set_stroke_colors(void* shapes[], const ShapeType types[],
                      size_t count, const Color& color);

void set_stroke_widths(void* shapes[], const ShapeType types[],
                      size_t count, float width);

/**
 * @brief Type-safe batch operations using templates
 */
template<typename ShapeT>
class TypedBatchOps {
public:
    // Transform a batch of the same shape type
    static void transform(ShapeT* shapes, size_t count, const Transform2D& transform) {
        ShapeT::batch_transform(shapes, count, transform);
    }
    
    // Calculate bounds for a batch
    static void calculate_bounds(const ShapeT* shapes, size_t count, BoundingBox* results) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            results[i] = shapes[i].get_bounds();
        }
    }
    
    // Point containment for a batch
    static void contains_points(const ShapeT* shapes, size_t count, 
                               const Point2D* points, bool* results) {
        ShapeT::batch_contains(shapes, count, points, results);
    }
    
    // Set colors for a batch
    static void set_fill_colors(ShapeT* shapes, size_t count, const Color& color) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            shapes[i].set_fill_color(color);
        }
    }
    
    static void set_stroke_colors(ShapeT* shapes, size_t count, const Color& color) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            shapes[i].set_stroke_color(color);
        }
    }
};

/**
 * @brief SIMD-optimized bounds calculation
 */
namespace simd {

// Calculate bounds for aligned arrays of circles
void calculate_circle_bounds_simd(const CircleShape* circles, size_t count, 
                                 BoundingBox* results);

// Calculate bounds for aligned arrays of rectangles
void calculate_rectangle_bounds_simd(const RectangleShape* rectangles, size_t count,
                                    BoundingBox* results);

// Calculate bounds for aligned arrays of ellipses
void calculate_ellipse_bounds_simd(const EllipseShape* ellipses, size_t count,
                                  BoundingBox* results);

// Calculate bounds for aligned arrays of lines
void calculate_line_bounds_simd(const LineShape* lines, size_t count,
                               BoundingBox* results);

} // namespace simd

/**
 * @brief Shape collection for heterogeneous batch operations
 */
class ShapeCollection {
private:
    std::vector<std::unique_ptr<void, void(*)(void*)>> shapes_;
    std::vector<ShapeType> types_;
    
public:
    ShapeCollection() = default;
    
    // Add shapes to collection
    void add_circle(std::unique_ptr<Circle> circle) {
        types_.push_back(ShapeType::Circle);
        shapes_.emplace_back(circle.release(), [](void* p) { delete static_cast<Circle*>(p); });
    }
    
    void add_rectangle(std::unique_ptr<Rectangle> rect) {
        types_.push_back(ShapeType::Rectangle);
        shapes_.emplace_back(rect.release(), [](void* p) { delete static_cast<Rectangle*>(p); });
    }
    
    void add_ellipse(std::unique_ptr<Ellipse> ellipse) {
        types_.push_back(ShapeType::Ellipse);
        shapes_.emplace_back(ellipse.release(), [](void* p) { delete static_cast<Ellipse*>(p); });
    }
    
    void add_line(std::unique_ptr<Line> line) {
        types_.push_back(ShapeType::Line);
        shapes_.emplace_back(line.release(), [](void* p) { delete static_cast<Line*>(p); });
    }
    
    // Batch operations on the collection
    void transform_all(const Transform2D& transform);
    std::vector<BoundingBox> calculate_all_bounds() const;
    BoundingBox calculate_union_bounds() const;
    std::vector<bool> contains_points(const std::vector<Point2D>& points) const;
    
    // Accessors
    size_t size() const { return shapes_.size(); }
    bool empty() const { return shapes_.empty(); }
    
    // Clear the collection
    void clear() {
        shapes_.clear();
        types_.clear();
    }
};

/**
 * @brief Performance utilities
 */
namespace perf {

// Prefetch shape data for better cache performance
template<typename ShapeT>
inline void prefetch_shapes(const ShapeT* shapes, size_t count, size_t ahead = 4) {
    const size_t limit = (count > ahead) ? count - ahead : 0;
    for (size_t i = 0; i < limit; ++i) {
        #ifdef __builtin_prefetch
        __builtin_prefetch(&shapes[i + ahead], 0, 1);
        #endif
    }
}

// Process shapes in cache-friendly chunks
template<typename ShapeT, typename Func>
void process_in_chunks(ShapeT* shapes, size_t count, size_t chunk_size, Func func) {
    const size_t num_chunks = (count + chunk_size - 1) / chunk_size;
    
    #pragma omp parallel for
    for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
        const size_t start = chunk * chunk_size;
        const size_t end = std::min(start + chunk_size, count);
        
        for (size_t i = start; i < end; ++i) {
            func(shapes[i], i);
        }
    }
}

} // namespace perf

} // namespace batch
} // namespace shapes
} // namespace claude_draw