#pragma once

#include "../core/bounding_box.h"
#include <atomic>
#include <mutex>
#include <array>
#include <algorithm>

namespace claude_draw {
namespace containers {

/**
 * @brief Incremental bounds tracker for efficient bounds updates
 * 
 * This class maintains bounds incrementally as shapes are added, removed,
 * or modified, avoiding full recalculation when possible.
 * 
 * Features:
 * - O(1) bounds expansion when adding shapes
 * - Deferred recalculation when removing shapes
 * - Thread-safe operations
 * - Dirty flag optimization
 */
class IncrementalBounds {
public:
    IncrementalBounds() 
        : bounds_()
        , dirty_(false)
        , shape_count_(0) {}
    
    /**
     * @brief Expand bounds to include a new shape
     * 
     * This is O(1) as it only expands the existing bounds.
     */
    void add_shape(const BoundingBox& shape_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (shape_count_ == 0) {
            bounds_ = shape_bounds;
        } else {
            bounds_ = bounds_.union_with(shape_bounds);
        }
        
        shape_count_++;
    }
    
    /**
     * @brief Mark bounds as dirty when a shape is removed
     * 
     * Actual recalculation is deferred until get_bounds() is called.
     */
    void remove_shape(const BoundingBox& shape_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        shape_count_--;
        
        // If the removed shape was at the edge of bounds, mark dirty
        if (shape_count_ > 0 && 
            (shape_bounds.min_x <= bounds_.min_x ||
             shape_bounds.min_y <= bounds_.min_y ||
             shape_bounds.max_x >= bounds_.max_x ||
             shape_bounds.max_y >= bounds_.max_y)) {
            dirty_ = true;
        } else if (shape_count_ == 0) {
            bounds_ = BoundingBox(0, 0, 0, 0);
            dirty_ = false;
        }
    }
    
    /**
     * @brief Update bounds when a shape is modified
     * 
     * @param old_bounds The previous bounds of the shape
     * @param new_bounds The new bounds of the shape
     */
    void update_shape(const BoundingBox& old_bounds, const BoundingBox& new_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // If new bounds are within current bounds, no update needed
        if (!dirty_ && bounds_.contains(new_bounds)) {
            return;
        }
        
        // If old bounds were at edge, mark dirty
        if (old_bounds.min_x <= bounds_.min_x ||
            old_bounds.min_y <= bounds_.min_y ||
            old_bounds.max_x >= bounds_.max_x ||
            old_bounds.max_y >= bounds_.max_y) {
            dirty_ = true;
        }
        
        // Expand to include new bounds
        bounds_ = bounds_.union_with(new_bounds);
    }
    
    /**
     * @brief Get current bounds (may trigger recalculation)
     * 
     * @return Current bounding box
     */
    BoundingBox get_bounds() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shape_count_ == 0) {
            return BoundingBox(0, 0, 0, 0);
        }
        return bounds_;
    }
    
    /**
     * @brief Check if bounds need recalculation
     */
    bool is_dirty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return dirty_;
    }
    
    /**
     * @brief Force bounds recalculation
     * 
     * @param new_bounds The recalculated bounds
     */
    void set_bounds(const BoundingBox& new_bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        bounds_ = new_bounds;
        dirty_ = false;
    }
    
    /**
     * @brief Clear all bounds
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        bounds_ = BoundingBox(0, 0, 0, 0);
        dirty_ = false;
        shape_count_ = 0;
    }
    
    /**
     * @brief Get the number of shapes tracked
     */
    size_t shape_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shape_count_;
    }
    
    /**
     * @brief Mark bounds as dirty for forced recalculation
     */
    void mark_dirty() {
        std::lock_guard<std::mutex> lock(mutex_);
        dirty_ = true;
    }
    
private:
    mutable std::mutex mutex_;
    BoundingBox bounds_;
    std::atomic<bool> dirty_;
    size_t shape_count_;
};

/**
 * @brief Hierarchical bounds tracker for spatial partitioning
 * 
 * Maintains bounds for spatial regions to enable efficient partial updates.
 */
class HierarchicalBounds {
public:
    static constexpr size_t GRID_SIZE = 8;  // 8x8 grid
    
    HierarchicalBounds() {
        clear();
    }
    
    /**
     * @brief Add shape to appropriate grid cell
     */
    void add_shape(const BoundingBox& shape_bounds) {
        size_t cell_x, cell_y;
        get_cell_indices(shape_bounds, cell_x, cell_y);
        
        cells_[cell_y * GRID_SIZE + cell_x].add_shape(shape_bounds);
        global_bounds_.add_shape(shape_bounds);
    }
    
    /**
     * @brief Remove shape from grid
     */
    void remove_shape(const BoundingBox& shape_bounds) {
        size_t cell_x, cell_y;
        get_cell_indices(shape_bounds, cell_x, cell_y);
        
        cells_[cell_y * GRID_SIZE + cell_x].remove_shape(shape_bounds);
        global_bounds_.mark_dirty();  // May need recalculation
    }
    
    /**
     * @brief Get bounds for a specific region
     */
    BoundingBox get_region_bounds(const BoundingBox& region) const {
        BoundingBox result;
        
        // Find cells that intersect with region
        size_t min_cell_x, min_cell_y, max_cell_x, max_cell_y;
        get_cell_range(region, min_cell_x, min_cell_y, max_cell_x, max_cell_y);
        
        for (size_t y = min_cell_y; y <= max_cell_y; ++y) {
            for (size_t x = min_cell_x; x <= max_cell_x; ++x) {
                const auto& cell = cells_[y * GRID_SIZE + x];
                if (cell.shape_count() > 0) {
                    result = result.union_with(cell.get_bounds());
                }
            }
        }
        
        return result;
    }
    
    /**
     * @brief Get global bounds
     */
    BoundingBox get_bounds() const {
        if (global_bounds_.is_dirty()) {
            // Recalculate from cells
            BoundingBox new_bounds;
            bool found_any = false;
            for (const auto& cell : cells_) {
                if (cell.shape_count() > 0) {
                    if (!found_any) {
                        new_bounds = cell.get_bounds();
                        found_any = true;
                    } else {
                        new_bounds = new_bounds.union_with(cell.get_bounds());
                    }
                }
            }
            if (!found_any) {
                new_bounds = BoundingBox(0, 0, 0, 0);
            }
            global_bounds_.set_bounds(new_bounds);
        }
        return global_bounds_.get_bounds();
    }
    
    /**
     * @brief Clear all bounds
     */
    void clear() {
        for (auto& cell : cells_) {
            cell.clear();
        }
        global_bounds_.clear();
        
        // Initialize grid bounds
        float cell_width = 1000.0f;  // Assume 8000x8000 space
        float cell_height = 1000.0f;
        
        grid_bounds_ = BoundingBox(0.0f, 0.0f, 
                                  cell_width * GRID_SIZE, 
                                  cell_height * GRID_SIZE);
    }
    
private:
    std::array<IncrementalBounds, GRID_SIZE * GRID_SIZE> cells_;
    mutable IncrementalBounds global_bounds_;
    BoundingBox grid_bounds_;
    
    void get_cell_indices(const BoundingBox& bounds, size_t& cell_x, size_t& cell_y) const {
        float cx = bounds.center_x();
        float cy = bounds.center_y();
        
        float cell_width = grid_bounds_.width() / GRID_SIZE;
        float cell_height = grid_bounds_.height() / GRID_SIZE;
        
        cell_x = static_cast<size_t>(std::clamp(cx / cell_width, 0.0f, GRID_SIZE - 1.0f));
        cell_y = static_cast<size_t>(std::clamp(cy / cell_height, 0.0f, GRID_SIZE - 1.0f));
    }
    
    void get_cell_range(const BoundingBox& bounds, 
                       size_t& min_x, size_t& min_y,
                       size_t& max_x, size_t& max_y) const {
        float cell_width = grid_bounds_.width() / GRID_SIZE;
        float cell_height = grid_bounds_.height() / GRID_SIZE;
        
        min_x = static_cast<size_t>(std::clamp(bounds.min_x / cell_width, 0.0f, GRID_SIZE - 1.0f));
        min_y = static_cast<size_t>(std::clamp(bounds.min_y / cell_height, 0.0f, GRID_SIZE - 1.0f));
        max_x = static_cast<size_t>(std::clamp(bounds.max_x / cell_width, 0.0f, GRID_SIZE - 1.0f));
        max_y = static_cast<size_t>(std::clamp(bounds.max_y / cell_height, 0.0f, GRID_SIZE - 1.0f));
    }
};

} // namespace containers
} // namespace claude_draw