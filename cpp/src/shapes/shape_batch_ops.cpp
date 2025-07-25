#include "claude_draw/shapes/shape_batch_ops.h"
#include <algorithm>
#include <limits>

namespace claude_draw {
namespace shapes {
namespace batch {

// Helper to dispatch operations based on shape type
template<typename Func>
void dispatch_shape_op(void* shape, ShapeType type, Func func) {
    switch (type) {
        case ShapeType::Circle:
            func(*static_cast<Circle*>(shape));
            break;
        case ShapeType::Rectangle:
            func(*static_cast<Rectangle*>(shape));
            break;
        case ShapeType::Ellipse:
            func(*static_cast<Ellipse*>(shape));
            break;
        case ShapeType::Line:
            func(*static_cast<Line*>(shape));
            break;
        default:
            break;
    }
}

template<typename Func>
void dispatch_const_shape_op(const void* shape, ShapeType type, Func func) {
    switch (type) {
        case ShapeType::Circle:
            func(*static_cast<const Circle*>(shape));
            break;
        case ShapeType::Rectangle:
            func(*static_cast<const Rectangle*>(shape));
            break;
        case ShapeType::Ellipse:
            func(*static_cast<const Ellipse*>(shape));
            break;
        case ShapeType::Line:
            func(*static_cast<const Line*>(shape));
            break;
        default:
            break;
    }
}

// Batch bounding box calculation
void calculate_bounds(const void* shapes[], const ShapeType types[], 
                     size_t count, BoundingBox results[]) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        dispatch_const_shape_op(shapes[i], types[i], 
            [&results, i](const auto& shape) {
                results[i] = shape.get_bounds();
            });
    }
}

// Batch transformation
void transform_shapes(void* shapes[], const ShapeType types[], 
                     size_t count, const Transform2D& transform) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        dispatch_shape_op(shapes[i], types[i], 
            [&transform](auto& shape) {
                shape.transform(transform);
            });
    }
}

// Batch point containment
void contains_points(const void* shapes[], const ShapeType types[],
                    size_t count, const Point2D points[], bool results[]) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        dispatch_const_shape_op(shapes[i], types[i], 
            [&points, &results, i](const auto& shape) {
                results[i] = shape.contains_point(points[i]);
            });
    }
}

// Calculate union bounds
BoundingBox calculate_union_bounds(const void* shapes[], const ShapeType types[], 
                                  size_t count) {
    if (count == 0) {
        return BoundingBox(0, 0, 0, 0);
    }
    
    // Initialize with first shape's bounds
    BoundingBox result;
    dispatch_const_shape_op(shapes[0], types[0], 
        [&result](const auto& shape) {
            result = shape.get_bounds();
        });
    
    // Extend with remaining shapes
    for (size_t i = 1; i < count; ++i) {
        dispatch_const_shape_op(shapes[i], types[i], 
            [&result](const auto& shape) {
                BoundingBox bounds = shape.get_bounds();
                result.min_x = std::min(result.min_x, bounds.min_x);
                result.min_y = std::min(result.min_y, bounds.min_y);
                result.max_x = std::max(result.max_x, bounds.max_x);
                result.max_y = std::max(result.max_y, bounds.max_y);
            });
    }
    
    return result;
}

// Filter visible shapes
size_t filter_visible(const void* shapes[], const ShapeType types[],
                     size_t count, void* visible_shapes[], ShapeType visible_types[]) {
    size_t visible_count = 0;
    
    for (size_t i = 0; i < count; ++i) {
        bool is_visible = false;
        dispatch_const_shape_op(shapes[i], types[i], 
            [&is_visible](const auto& shape) {
                is_visible = shape.is_visible();
            });
        
        if (is_visible) {
            visible_shapes[visible_count] = const_cast<void*>(shapes[i]);
            visible_types[visible_count] = types[i];
            visible_count++;
        }
    }
    
    return visible_count;
}

// Batch color operations
void set_fill_colors(void* shapes[], const ShapeType types[],
                    size_t count, const Color& color) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        // Lines don't have fill color
        if (types[i] != ShapeType::Line) {
            dispatch_shape_op(shapes[i], types[i], 
                [&color](auto& shape) {
                    shape.set_fill_color(color);
                });
        }
    }
}

void set_stroke_colors(void* shapes[], const ShapeType types[],
                      size_t count, const Color& color) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        dispatch_shape_op(shapes[i], types[i], 
            [&color](auto& shape) {
                shape.set_stroke_color(color);
            });
    }
}

void set_stroke_widths(void* shapes[], const ShapeType types[],
                      size_t count, float width) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        dispatch_shape_op(shapes[i], types[i], 
            [width](auto& shape) {
                shape.set_stroke_width(width);
            });
    }
}

// SIMD implementations
namespace simd {

#ifdef __AVX2__

void calculate_circle_bounds_simd(const CircleShape* circles, size_t count, 
                                 BoundingBox* results) {
    const size_t simd_count = count & ~3; // Process 4 at a time
    
    for (size_t i = 0; i < simd_count; i += 4) {
        // Load 4 circles' data
        __m256 cx = _mm256_set_ps(circles[i+3].center_x, circles[i+2].center_x,
                                  circles[i+1].center_x, circles[i].center_x,
                                  circles[i+3].center_x, circles[i+2].center_x,
                                  circles[i+1].center_x, circles[i].center_x);
        
        __m256 cy = _mm256_set_ps(circles[i+3].center_y, circles[i+2].center_y,
                                  circles[i+1].center_y, circles[i].center_y,
                                  circles[i+3].center_y, circles[i+2].center_y,
                                  circles[i+1].center_y, circles[i].center_y);
        
        __m256 r = _mm256_set_ps(circles[i+3].radius, circles[i+2].radius,
                                 circles[i+1].radius, circles[i].radius,
                                 circles[i+3].radius, circles[i+2].radius,
                                 circles[i+1].radius, circles[i].radius);
        
        // Calculate bounds
        __m256 min_x = _mm256_sub_ps(cx, r);
        __m256 max_x = _mm256_add_ps(cx, r);
        __m256 min_y = _mm256_sub_ps(cy, r);
        __m256 max_y = _mm256_add_ps(cy, r);
        
        // Store results
        float min_x_arr[8], max_x_arr[8], min_y_arr[8], max_y_arr[8];
        _mm256_storeu_ps(min_x_arr, min_x);
        _mm256_storeu_ps(max_x_arr, max_x);
        _mm256_storeu_ps(min_y_arr, min_y);
        _mm256_storeu_ps(max_y_arr, max_y);
        
        for (size_t j = 0; j < 4; ++j) {
            results[i + j] = BoundingBox(min_x_arr[j], min_y_arr[j], 
                                        max_x_arr[j], max_y_arr[j]);
        }
    }
    
    // Handle remaining circles
    for (size_t i = simd_count; i < count; ++i) {
        results[i] = circles[i].bounds();
    }
}

#else

// Fallback non-SIMD implementation
void calculate_circle_bounds_simd(const CircleShape* circles, size_t count, 
                                 BoundingBox* results) {
    for (size_t i = 0; i < count; ++i) {
        results[i] = circles[i].bounds();
    }
}

#endif

// Similar implementations for other shapes...
void calculate_rectangle_bounds_simd(const RectangleShape* rectangles, size_t count,
                                    BoundingBox* results) {
    // Rectangles already store bounds directly
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = rectangles[i].bounds();
    }
}

void calculate_ellipse_bounds_simd(const EllipseShape* ellipses, size_t count,
                                  BoundingBox* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = ellipses[i].bounds();
    }
}

void calculate_line_bounds_simd(const LineShape* lines, size_t count,
                               BoundingBox* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = lines[i].bounds();
    }
}

} // namespace simd

// ShapeCollection implementation
void ShapeCollection::transform_all(const Transform2D& transform) {
    for (size_t i = 0; i < shapes_.size(); ++i) {
        dispatch_shape_op(shapes_[i].get(), types_[i], 
            [&transform](auto& shape) {
                shape.transform(transform);
            });
    }
}

std::vector<BoundingBox> ShapeCollection::calculate_all_bounds() const {
    std::vector<BoundingBox> results(shapes_.size());
    
    for (size_t i = 0; i < shapes_.size(); ++i) {
        dispatch_const_shape_op(shapes_[i].get(), types_[i], 
            [&results, i](const auto& shape) {
                results[i] = shape.get_bounds();
            });
    }
    
    return results;
}

BoundingBox ShapeCollection::calculate_union_bounds() const {
    if (shapes_.empty()) {
        return BoundingBox(0, 0, 0, 0);
    }
    
    BoundingBox result;
    dispatch_const_shape_op(shapes_[0].get(), types_[0], 
        [&result](const auto& shape) {
            result = shape.get_bounds();
        });
    
    for (size_t i = 1; i < shapes_.size(); ++i) {
        dispatch_const_shape_op(shapes_[i].get(), types_[i], 
            [&result](const auto& shape) {
                BoundingBox bounds = shape.get_bounds();
                result.min_x = std::min(result.min_x, bounds.min_x);
                result.min_y = std::min(result.min_y, bounds.min_y);
                result.max_x = std::max(result.max_x, bounds.max_x);
                result.max_y = std::max(result.max_y, bounds.max_y);
            });
    }
    
    return result;
}

std::vector<bool> ShapeCollection::contains_points(const std::vector<Point2D>& points) const {
    if (points.size() != shapes_.size()) {
        throw std::invalid_argument("Points vector size must match shapes collection size");
    }
    
    std::vector<bool> results(shapes_.size());
    
    #pragma omp parallel for
    for (size_t i = 0; i < shapes_.size(); ++i) {
        dispatch_const_shape_op(shapes_[i].get(), types_[i], 
            [&results, &points, i](const auto& shape) {
                results[i] = shape.contains_point(points[i]);
            });
    }
    
    return results;
}

} // namespace batch
} // namespace shapes
} // namespace claude_draw