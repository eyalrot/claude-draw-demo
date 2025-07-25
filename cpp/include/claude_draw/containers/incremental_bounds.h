#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <array>
#include "../core/bounding_box.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Incrementally tracks bounding box for a collection of shapes
 * 
 * Optimizes bounds calculation by:
 * - O(1) addition when shape doesn't extend current bounds
 * - Lazy recalculation only when necessary (shape removal at bounds edge)
 * - Thread-safe operations for concurrent access
 */
class IncrementalBounds {
public:
    IncrementalBounds() : shape_count_(0), is_dirty_(false) {
        // Initialize to zero bounds for empty state
        bounds_ = BoundingBox(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    /**
     * @brief Add a shape's bounding box
     * O(1) operation - just extends current bounds
     */
    void add_shape(const BoundingBox& shape_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (shape_count_ == 0) {
            bounds_ = shape_bounds;
        } else {
            bounds_ = bounds_.merge(shape_bounds);
        }
        
        shape_count_++;
        is_dirty_ = false;
    }
    
    /**
     * @brief Remove a shape's bounding box
     * May mark bounds as dirty if shape was at edge
     */
    void remove_shape(const BoundingBox& shape_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (shape_count_ == 0) return;
        
        shape_count_--;
        
        if (shape_count_ == 0) {
            bounds_ = BoundingBox(0.0f, 0.0f, 0.0f, 0.0f);
            is_dirty_ = false;
            return;
        }
        
        // Check if removed shape was at bounds edge
        const float epsilon = 0.0001f;
        bool at_edge = (std::abs(shape_bounds.min_x - bounds_.min_x) < epsilon ||
                       std::abs(shape_bounds.min_y - bounds_.min_y) < epsilon ||
                       std::abs(shape_bounds.max_x - bounds_.max_x) < epsilon ||
                       std::abs(shape_bounds.max_y - bounds_.max_y) < epsilon);
        
        if (at_edge) {
            is_dirty_ = true;
        }
    }
    
    /**
     * @brief Update a shape's bounding box
     * Efficiently handles shape movement
     */
    void update_shape(const BoundingBox& old_bounds, const BoundingBox& new_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // If new bounds are within old bounds, no update needed
        if (old_bounds.contains(new_bounds)) {
            return;
        }
        
        // If new bounds extend current bounds, just extend
        if (!is_dirty_) {
            bounds_ = bounds_.merge(new_bounds);
        }
        
        // Check if old bounds were at edge
        const float epsilon = 0.0001f;
        bool was_at_edge = (std::abs(old_bounds.min_x - bounds_.min_x) < epsilon ||
                           std::abs(old_bounds.min_y - bounds_.min_y) < epsilon ||
                           std::abs(old_bounds.max_x - bounds_.max_x) < epsilon ||
                           std::abs(old_bounds.max_y - bounds_.max_y) < epsilon);
        
        if (was_at_edge && !bounds_.contains(new_bounds)) {
            is_dirty_ = true;
        }
    }
    
    /**
     * @brief Get current bounds (const access)
     */
    BoundingBox get_bounds() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return bounds_;
    }
    
    /**
     * @brief Set bounds (used after recalculation)
     */
    void set_bounds(const BoundingBox& new_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        bounds_ = new_bounds;
        is_dirty_ = false;
    }
    
    /**
     * @brief Check if bounds need recalculation
     */
    bool is_dirty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_dirty_;
    }
    
    /**
     * @brief Mark bounds as needing recalculation
     */
    void mark_dirty() {
        std::lock_guard<std::mutex> lock(mutex_);
        is_dirty_ = true;
    }
    
    /**
     * @brief Get number of shapes
     */
    size_t shape_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shape_count_;
    }
    
    /**
     * @brief Clear all tracking
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        bounds_ = BoundingBox(0.0f, 0.0f, 0.0f, 0.0f);
        shape_count_ = 0;
        is_dirty_ = false;
    }
    
private:
    mutable std::mutex mutex_;
    BoundingBox bounds_;
    size_t shape_count_;
    bool is_dirty_;
};

/**
 * @brief Hierarchical bounds tracking for large scenes
 * 
 * Divides space into a grid and tracks bounds per cell.
 * Optimizes queries for viewport culling and partial updates.
 */
class HierarchicalBounds {
public:
    static constexpr size_t GRID_SIZE = 16;  // 16x16 grid
    static constexpr float CELL_SIZE = 512.0f;  // Each cell covers 512x512 units
    
    HierarchicalBounds() {
        clear();
    }
    
    /**
     * @brief Add shape to appropriate grid cell(s)
     */
    void add_shape(const BoundingBox& shape_bounds) {
        // Determine which cells the shape overlaps
        int min_cell_x = static_cast<int>(shape_bounds.min_x / CELL_SIZE);
        int min_cell_y = static_cast<int>(shape_bounds.min_y / CELL_SIZE);
        int max_cell_x = static_cast<int>(shape_bounds.max_x / CELL_SIZE);
        int max_cell_y = static_cast<int>(shape_bounds.max_y / CELL_SIZE);
        
        // Clamp to grid bounds
        min_cell_x = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), min_cell_x));
        min_cell_y = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), min_cell_y));
        max_cell_x = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), max_cell_x));
        max_cell_y = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), max_cell_y));
        
        // Update cells
        for (int y = min_cell_y; y <= max_cell_y; ++y) {
            for (int x = min_cell_x; x <= max_cell_x; ++x) {
                cells_[y][x].add_shape(shape_bounds);
            }
        }
        
        // Update global bounds
        global_bounds_.add_shape(shape_bounds);
    }
    
    /**
     * @brief Remove shape from grid cells
     */
    void remove_shape(const BoundingBox& shape_bounds) {
        // Determine which cells the shape overlaps
        int min_cell_x = static_cast<int>(shape_bounds.min_x / CELL_SIZE);
        int min_cell_y = static_cast<int>(shape_bounds.min_y / CELL_SIZE);
        int max_cell_x = static_cast<int>(shape_bounds.max_x / CELL_SIZE);
        int max_cell_y = static_cast<int>(shape_bounds.max_y / CELL_SIZE);
        
        // Clamp to grid bounds
        min_cell_x = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), min_cell_x));
        min_cell_y = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), min_cell_y));
        max_cell_x = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), max_cell_x));
        max_cell_y = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), max_cell_y));
        
        // Update cells
        for (int y = min_cell_y; y <= max_cell_y; ++y) {
            for (int x = min_cell_x; x <= max_cell_x; ++x) {
                cells_[y][x].remove_shape(shape_bounds);
            }
        }
        
        // Update global bounds
        global_bounds_.remove_shape(shape_bounds);
    }
    
    /**
     * @brief Get bounds for entire scene
     */
    BoundingBox get_bounds() const {
        return global_bounds_.get_bounds();
    }
    
    /**
     * @brief Get bounds for specific region (e.g., viewport)
     */
    BoundingBox get_region_bounds(const BoundingBox& region) const {
        // Determine which cells the region overlaps
        int min_cell_x = static_cast<int>(region.min_x / CELL_SIZE);
        int min_cell_y = static_cast<int>(region.min_y / CELL_SIZE);
        int max_cell_x = static_cast<int>(region.max_x / CELL_SIZE);
        int max_cell_y = static_cast<int>(region.max_y / CELL_SIZE);
        
        // Clamp to grid bounds
        min_cell_x = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), min_cell_x));
        min_cell_y = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), min_cell_y));
        max_cell_x = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), max_cell_x));
        max_cell_y = std::max(0, std::min(static_cast<int>(GRID_SIZE - 1), max_cell_y));
        
        // Merge bounds from relevant cells
        BoundingBox result;
        bool first = true;
        
        for (int y = min_cell_y; y <= max_cell_y; ++y) {
            for (int x = min_cell_x; x <= max_cell_x; ++x) {
                if (cells_[y][x].shape_count() > 0) {
                    BoundingBox cell_bounds = cells_[y][x].get_bounds();
                    if (first) {
                        result = cell_bounds;
                        first = false;
                    } else {
                        result = result.merge(cell_bounds);
                    }
                }
            }
        }
        
        return result;
    }
    
    /**
     * @brief Clear all bounds
     */
    void clear() {
        for (size_t y = 0; y < GRID_SIZE; ++y) {
            for (size_t x = 0; x < GRID_SIZE; ++x) {
                cells_[y][x].clear();
            }
        }
        global_bounds_.clear();
    }
    
private:
    std::array<std::array<IncrementalBounds, GRID_SIZE>, GRID_SIZE> cells_;
    IncrementalBounds global_bounds_;
};

} // namespace containers
} // namespace claude_draw