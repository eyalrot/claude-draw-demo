#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <numeric>
#include "../shapes/shape_base_optimized.h"
#include "../shapes/circle_optimized.h"
#include "../shapes/rectangle_optimized.h"
#include "../shapes/ellipse_optimized.h"
#include "../shapes/line_optimized.h"
#include "../core/simd.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Structure of Arrays container for shapes
 * 
 * This container stores shapes in separate arrays by type to enable:
 * - SIMD processing of homogeneous data
 * - Better cache locality for batch operations
 * - Efficient memory layout for parallel processing
 * 
 * Memory Layout:
 * - Each shape type has its own contiguous array
 * - Arrays are aligned for SIMD access
 * - Indices map to positions in type-specific arrays
 */
class SoAContainer {
public:
    // Type aliases for clarity
    using Index = uint32_t;
    using ShapeHandle = std::pair<shapes::ShapeType, Index>;
    
    // Constants
    static constexpr size_t INITIAL_CAPACITY = 1024;
    static constexpr size_t SIMD_ALIGNMENT = 32;  // AVX2 alignment
    
private:
    // Structure to manage a type-specific array
    template<typename ShapeT>
    struct TypedArray {
        std::vector<ShapeT> data;
        std::vector<Index> free_list;  // Indices of deleted elements
        size_t count = 0;
        
        TypedArray() {
            data.reserve(INITIAL_CAPACITY);
        }
        
        // Add a shape and return its index
        Index add(const ShapeT& shape) {
            Index idx;
            if (!free_list.empty()) {
                idx = free_list.back();
                free_list.pop_back();
                data[idx] = shape;
            } else {
                idx = static_cast<Index>(data.size());
                data.push_back(shape);
            }
            count++;
            return idx;
        }
        
        // Remove a shape by marking its index as free
        void remove(Index idx) {
            if (idx < data.size() && count > 0) {
                free_list.push_back(idx);
                count--;
            }
        }
        
        // Get shape at index (no bounds checking in release)
        ShapeT& operator[](Index idx) { return data[idx]; }
        const ShapeT& operator[](Index idx) const { return data[idx]; }
        
        // Check if index is valid (not in free list)
        bool is_valid(Index idx) const {
            return idx < data.size() && 
                   std::find(free_list.begin(), free_list.end(), idx) == free_list.end();
        }
        
        // Compact the array by removing free slots
        void compact() {
            if (free_list.empty()) return;
            
            // Sort free list in descending order
            std::sort(free_list.begin(), free_list.end(), std::greater<Index>());
            
            // Remove elements from back to preserve indices
            for (Index idx : free_list) {
                if (idx == data.size() - 1) {
                    data.pop_back();
                }
            }
            
            // Clear processed free indices
            free_list.erase(
                std::remove_if(free_list.begin(), free_list.end(),
                    [this](Index idx) { return idx >= data.size(); }),
                free_list.end()
            );
        }
    };
    
    // Type-specific arrays
    TypedArray<shapes::CircleShape> circles_;
    TypedArray<shapes::RectangleShape> rectangles_;
    TypedArray<shapes::EllipseShape> ellipses_;
    TypedArray<shapes::LineShape> lines_;
    
    // Global shape registry
    struct ShapeEntry {
        shapes::ShapeType type;
        Index type_index;
        uint32_t id;
        uint16_t z_index;
        bool visible;
    };
    
    std::vector<ShapeEntry> shape_registry_;
    std::unordered_map<uint32_t, size_t> id_to_registry_index_;
    uint32_t next_id_ = 1;
    
    // Bounds caching
    mutable BoundingBox cached_bounds_;
    mutable bool bounds_dirty_ = true;
    
public:
    SoAContainer() {
        shape_registry_.reserve(INITIAL_CAPACITY);
    }
    
    // Add shape methods
    uint32_t add_circle(const shapes::Circle& circle) {
        Index idx = circles_.add(circle.data());
        return add_to_registry(shapes::ShapeType::Circle, idx);
    }
    
    uint32_t add_rectangle(const shapes::Rectangle& rect) {
        Index idx = rectangles_.add(rect.data());
        return add_to_registry(shapes::ShapeType::Rectangle, idx);
    }
    
    uint32_t add_ellipse(const shapes::Ellipse& ellipse) {
        Index idx = ellipses_.add(ellipse.data());
        return add_to_registry(shapes::ShapeType::Ellipse, idx);
    }
    
    uint32_t add_line(const shapes::Line& line) {
        Index idx = lines_.add(line.data());
        return add_to_registry(shapes::ShapeType::Line, idx);
    }
    
    // Remove shape by ID
    bool remove(uint32_t id) {
        auto it = id_to_registry_index_.find(id);
        if (it == id_to_registry_index_.end()) {
            return false;
        }
        
        size_t reg_idx = it->second;
        const ShapeEntry& entry = shape_registry_[reg_idx];
        
        // Remove from type-specific array
        switch (entry.type) {
            case shapes::ShapeType::Circle:
                circles_.remove(entry.type_index);
                break;
            case shapes::ShapeType::Rectangle:
                rectangles_.remove(entry.type_index);
                break;
            case shapes::ShapeType::Ellipse:
                ellipses_.remove(entry.type_index);
                break;
            case shapes::ShapeType::Line:
                lines_.remove(entry.type_index);
                break;
        }
        
        // Remove from registry
        id_to_registry_index_.erase(it);
        shape_registry_.erase(shape_registry_.begin() + reg_idx);
        
        // Update registry indices
        for (size_t i = reg_idx; i < shape_registry_.size(); ++i) {
            id_to_registry_index_[shape_registry_[i].id] = i;
        }
        
        bounds_dirty_ = true;
        return true;
    }
    
    // Access shapes by ID
    template<typename ShapeT>
    ShapeT* get(uint32_t id) {
        auto it = id_to_registry_index_.find(id);
        if (it == id_to_registry_index_.end()) {
            return nullptr;
        }
        
        const ShapeEntry& entry = shape_registry_[it->second];
        return get_typed_shape<ShapeT>(entry);
    }
    
    // Batch operations on all shapes of a type
    template<typename ShapeT, typename Func>
    void for_each_type(Func func) {
        if constexpr (std::is_same_v<ShapeT, shapes::CircleShape>) {
            for (Index i = 0; i < circles_.data.size(); ++i) {
                if (circles_.is_valid(i)) {
                    func(circles_[i]);
                }
            }
        } else if constexpr (std::is_same_v<ShapeT, shapes::RectangleShape>) {
            for (Index i = 0; i < rectangles_.data.size(); ++i) {
                if (rectangles_.is_valid(i)) {
                    func(rectangles_[i]);
                }
            }
        } else if constexpr (std::is_same_v<ShapeT, shapes::EllipseShape>) {
            for (Index i = 0; i < ellipses_.data.size(); ++i) {
                if (ellipses_.is_valid(i)) {
                    func(ellipses_[i]);
                }
            }
        } else if constexpr (std::is_same_v<ShapeT, shapes::LineShape>) {
            for (Index i = 0; i < lines_.data.size(); ++i) {
                if (lines_.is_valid(i)) {
                    func(lines_[i]);
                }
            }
        }
    }
    
    // Iterate all shapes in z-order
    template<typename Func>
    void for_each_sorted(Func func) {
        // Sort registry by z-index
        std::vector<size_t> sorted_indices(shape_registry_.size());
        std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
        
        std::sort(sorted_indices.begin(), sorted_indices.end(),
            [this](size_t a, size_t b) {
                return shape_registry_[a].z_index < shape_registry_[b].z_index;
            });
        
        // Visit shapes in z-order
        for (size_t idx : sorted_indices) {
            const ShapeEntry& entry = shape_registry_[idx];
            if (!entry.visible) continue;
            
            visit_shape(entry, func);
        }
    }
    
    // Get total shape count
    size_t size() const {
        return shape_registry_.size();
    }
    
    // Get count by type
    size_t count(shapes::ShapeType type) const {
        switch (type) {
            case shapes::ShapeType::Circle:
                return circles_.count;
            case shapes::ShapeType::Rectangle:
                return rectangles_.count;
            case shapes::ShapeType::Ellipse:
                return ellipses_.count;
            case shapes::ShapeType::Line:
                return lines_.count;
            default:
                return 0;
        }
    }
    
    // Memory management
    void reserve(size_t capacity) {
        shape_registry_.reserve(capacity);
        circles_.data.reserve(capacity / 4);
        rectangles_.data.reserve(capacity / 4);
        ellipses_.data.reserve(capacity / 4);
        lines_.data.reserve(capacity / 4);
    }
    
    void compact() {
        circles_.compact();
        rectangles_.compact();
        ellipses_.compact();
        lines_.compact();
    }
    
    // Bounds calculation
    BoundingBox get_bounds() const {
        if (!bounds_dirty_) {
            return cached_bounds_;
        }
        
        if (shape_registry_.empty()) {
            cached_bounds_ = BoundingBox(0, 0, 0, 0);
            bounds_dirty_ = false;
            return cached_bounds_;
        }
        
        // Initialize with first visible shape
        bool found_first = false;
        
        for (const auto& entry : shape_registry_) {
            if (!entry.visible) continue;
            
            BoundingBox shape_bounds = get_shape_bounds(entry);
            
            if (!found_first) {
                cached_bounds_ = shape_bounds;
                found_first = true;
            } else {
                cached_bounds_.min_x = std::min(cached_bounds_.min_x, shape_bounds.min_x);
                cached_bounds_.min_y = std::min(cached_bounds_.min_y, shape_bounds.min_y);
                cached_bounds_.max_x = std::max(cached_bounds_.max_x, shape_bounds.max_x);
                cached_bounds_.max_y = std::max(cached_bounds_.max_y, shape_bounds.max_y);
            }
        }
        
        if (!found_first) {
            cached_bounds_ = BoundingBox(0, 0, 0, 0);
        }
        
        bounds_dirty_ = false;
        return cached_bounds_;
    }
    
    // Clear all shapes
    void clear() {
        circles_ = TypedArray<shapes::CircleShape>();
        rectangles_ = TypedArray<shapes::RectangleShape>();
        ellipses_ = TypedArray<shapes::EllipseShape>();
        lines_ = TypedArray<shapes::LineShape>();
        shape_registry_.clear();
        id_to_registry_index_.clear();
        next_id_ = 1;
        bounds_dirty_ = true;
    }
    
    // Get direct access to typed arrays for SIMD operations
    const std::vector<shapes::CircleShape>& get_circles() const { return circles_.data; }
    const std::vector<shapes::RectangleShape>& get_rectangles() const { return rectangles_.data; }
    const std::vector<shapes::EllipseShape>& get_ellipses() const { return ellipses_.data; }
    const std::vector<shapes::LineShape>& get_lines() const { return lines_.data; }
    
    // Check if a shape ID exists
    bool contains(uint32_t id) const {
        return id_to_registry_index_.find(id) != id_to_registry_index_.end();
    }
    
    // Set visibility
    bool set_visible(uint32_t id, bool visible) {
        auto it = id_to_registry_index_.find(id);
        if (it == id_to_registry_index_.end()) {
            return false;
        }
        
        shape_registry_[it->second].visible = visible;
        bounds_dirty_ = true;
        return true;
    }
    
    // Set z-index
    bool set_z_index(uint32_t id, uint16_t z_index) {
        auto it = id_to_registry_index_.find(id);
        if (it == id_to_registry_index_.end()) {
            return false;
        }
        
        shape_registry_[it->second].z_index = z_index;
        return true;
    }
    
private:
    // Add shape to registry
    uint32_t add_to_registry(shapes::ShapeType type, Index type_index) {
        uint32_t id = next_id_++;
        
        ShapeEntry entry;
        entry.type = type;
        entry.type_index = type_index;
        entry.id = id;
        entry.z_index = static_cast<uint16_t>(shape_registry_.size());
        entry.visible = true;
        
        id_to_registry_index_[id] = shape_registry_.size();
        shape_registry_.push_back(entry);
        
        bounds_dirty_ = true;
        return id;
    }
    
    // Get typed shape pointer
    template<typename ShapeT>
    ShapeT* get_typed_shape(const ShapeEntry& entry) {
        if constexpr (std::is_same_v<ShapeT, shapes::CircleShape>) {
            return (entry.type == shapes::ShapeType::Circle && circles_.is_valid(entry.type_index))
                ? &circles_[entry.type_index] : nullptr;
        } else if constexpr (std::is_same_v<ShapeT, shapes::RectangleShape>) {
            return (entry.type == shapes::ShapeType::Rectangle && rectangles_.is_valid(entry.type_index))
                ? &rectangles_[entry.type_index] : nullptr;
        } else if constexpr (std::is_same_v<ShapeT, shapes::EllipseShape>) {
            return (entry.type == shapes::ShapeType::Ellipse && ellipses_.is_valid(entry.type_index))
                ? &ellipses_[entry.type_index] : nullptr;
        } else if constexpr (std::is_same_v<ShapeT, shapes::LineShape>) {
            return (entry.type == shapes::ShapeType::Line && lines_.is_valid(entry.type_index))
                ? &lines_[entry.type_index] : nullptr;
        }
        return nullptr;
    }
    
    // Visit a shape with a functor
    template<typename Func>
    void visit_shape(const ShapeEntry& entry, Func func) {
        switch (entry.type) {
            case shapes::ShapeType::Circle:
                if (circles_.is_valid(entry.type_index)) {
                    func(shapes::Circle(circles_[entry.type_index]));
                }
                break;
            case shapes::ShapeType::Rectangle:
                if (rectangles_.is_valid(entry.type_index)) {
                    func(shapes::Rectangle(rectangles_[entry.type_index]));
                }
                break;
            case shapes::ShapeType::Ellipse:
                if (ellipses_.is_valid(entry.type_index)) {
                    func(shapes::Ellipse(ellipses_[entry.type_index]));
                }
                break;
            case shapes::ShapeType::Line:
                if (lines_.is_valid(entry.type_index)) {
                    func(shapes::Line(lines_[entry.type_index]));
                }
                break;
        }
    }
    
    // Get bounds of a shape
    BoundingBox get_shape_bounds(const ShapeEntry& entry) const {
        switch (entry.type) {
            case shapes::ShapeType::Circle:
                return circles_.is_valid(entry.type_index) 
                    ? circles_[entry.type_index].bounds() 
                    : BoundingBox();
            case shapes::ShapeType::Rectangle:
                return rectangles_.is_valid(entry.type_index)
                    ? rectangles_[entry.type_index].bounds()
                    : BoundingBox();
            case shapes::ShapeType::Ellipse:
                return ellipses_.is_valid(entry.type_index)
                    ? ellipses_[entry.type_index].bounds()
                    : BoundingBox();
            case shapes::ShapeType::Line:
                return lines_.is_valid(entry.type_index)
                    ? lines_[entry.type_index].bounds()
                    : BoundingBox();
            default:
                return BoundingBox();
        }
    }
};

} // namespace containers
} // namespace claude_draw