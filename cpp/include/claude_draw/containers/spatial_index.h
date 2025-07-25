#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <limits>
#include <functional>
#include "../core/bounding_box.h"
#include "../shapes/shape_base_optimized.h"

namespace claude_draw {
namespace containers {

/**
 * @brief R-tree spatial index for efficient spatial queries
 * 
 * This implementation uses a dynamic R-tree with quadratic split algorithm.
 * Optimized for 2D graphics operations with typical scene complexity.
 * 
 * Features:
 * - Bulk loading with Sort-Tile-Recursive (STR) algorithm
 * - Dynamic insertion and removal
 * - Range queries and nearest neighbor search
 * - Configurable node capacity for cache optimization
 */
template<typename T>
class RTree {
public:
    // Configuration constants
    static constexpr size_t MIN_ENTRIES = 2;
    static constexpr size_t MAX_ENTRIES = 8;  // Tuned for cache line efficiency
    
    using ValueType = T;
    using BoundsType = BoundingBox;
    using QueryCallback = std::function<bool(const T&)>;  // Return false to stop iteration
    
private:
    struct Node;
    using NodePtr = std::unique_ptr<Node>;
    
    struct Entry {
        BoundingBox bounds;
        T value;              // For leaf nodes
        Node* child;          // For internal nodes
        
        Entry() : bounds(), value{}, child(nullptr) {}
        Entry(const BoundingBox& b, const T& v) : bounds(b), value(v), child(nullptr) {}
        Entry(const BoundingBox& b, Node* c) : bounds(b), value{}, child(c) {}
        
        // Default copy constructor works fine
        Entry(const Entry&) = default;
        Entry& operator=(const Entry&) = default;
        
        ~Entry() = default;
    };
    
    struct Node {
        std::vector<Entry> entries;
        bool is_leaf;
        
        Node(bool leaf = true) : is_leaf(leaf) {
            entries.reserve(MAX_ENTRIES + 1);  // +1 for temporary overflow
        }
        
        ~Node() = default;
        
        BoundingBox calculate_bounds() const {
            if (entries.empty()) {
                return BoundingBox();
            }
            
            BoundingBox result = entries[0].bounds;
            for (size_t i = 1; i < entries.size(); ++i) {
                result = result.merge(entries[i].bounds);
            }
            return result;
        }
        
        bool is_full() const {
            return entries.size() >= MAX_ENTRIES;
        }
        
        bool is_underfull() const {
            return entries.size() < MIN_ENTRIES;
        }
    };
    
    NodePtr root_;
    size_t size_;
    
public:
    RTree() : root_(std::make_unique<Node>(true)), size_(0) {}
    
    // Insert a value with its bounding box
    void insert(const BoundingBox& bounds, const T& value) {
        Entry new_entry(bounds, value);
        
        NodePtr new_child;
        bool did_split = insert_recursive(root_.get(), new_entry, new_child);
        
        if (did_split) {
            // Root split, create new root
            NodePtr new_root = std::make_unique<Node>(false);
            
            // Calculate bounds before releasing ownership
            BoundingBox old_root_bounds = root_->calculate_bounds();
            BoundingBox new_child_bounds = new_child->calculate_bounds();
            
            Entry old_root_entry(old_root_bounds, root_.release());
            Entry new_child_entry(new_child_bounds, new_child.release());
            
            new_root->entries.push_back(old_root_entry);
            new_root->entries.push_back(new_child_entry);
            
            root_ = std::move(new_root);
        }
        
        size_++;
    }
    
    // Remove a value (returns true if found and removed)
    bool remove(const BoundingBox& bounds, const T& value) {
        bool removed = remove_recursive(root_.get(), bounds, value);
        
        if (removed) {
            size_--;
            
            // Handle root becoming empty
            if (!root_->is_leaf && root_->entries.size() == 1) {
                // Replace root with its only child
                NodePtr new_root(root_->entries[0].child);
                root_->entries[0].child = nullptr;  // Prevent double delete
                root_ = std::move(new_root);
            }
        }
        
        return removed;
    }
    
    // Query all values intersecting the given bounds
    void query(const BoundingBox& bounds, QueryCallback callback) const {
        query_recursive(root_.get(), bounds, callback);
    }
    
    // Query all values within range
    std::vector<T> query_range(const BoundingBox& bounds) const {
        std::vector<T> results;
        query(bounds, [&results](const T& value) {
            results.push_back(value);
            return true;  // Continue iteration
        });
        return results;
    }
    
    // Find nearest neighbor
    std::pair<T, float> nearest_neighbor(float x, float y) const {
        if (size_ == 0) {
            return {T{}, std::numeric_limits<float>::max()};
        }
        
        T best_value{};
        float best_distance = std::numeric_limits<float>::max();
        
        nearest_neighbor_recursive(root_.get(), x, y, best_value, best_distance);
        
        return {best_value, best_distance};
    }
    
    // Bulk load using Sort-Tile-Recursive (STR) algorithm
    void bulk_load(std::vector<std::pair<BoundingBox, T>>& items) {
        if (items.empty()) return;
        
        // Clear existing tree
        root_ = std::make_unique<Node>(true);
        size_ = 0;
        
        // Build tree using STR
        root_ = build_str(items, 0, items.size());
        size_ = items.size();
    }
    
    // Get tree statistics
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    size_t height() const {
        return calculate_height(root_.get());
    }
    
    size_t node_count() const {
        return count_nodes(root_.get());
    }
    
    // Clear all entries
    void clear() {
        root_ = std::make_unique<Node>(true);
        size_ = 0;
    }
    
private:
    // Recursive insertion
    bool insert_recursive(Node* node, Entry& entry, NodePtr& new_child) {
        if (node->is_leaf) {
            // Add entry to leaf
            node->entries.push_back(entry);
            
            if (node->is_full()) {
                // Split leaf node
                new_child = split_node(node);
                return true;
            }
            return false;
        } else {
            // Choose subtree with least enlargement
            size_t best_idx = choose_subtree(node, entry.bounds);
            
            NodePtr split_child;
            bool did_split = insert_recursive(
                node->entries[best_idx].child, 
                entry, 
                split_child
            );
            
            // Update bounds
            node->entries[best_idx].bounds = 
                node->entries[best_idx].child->calculate_bounds();
            
            if (did_split) {
                // Add split child to this node
                BoundingBox split_bounds = split_child->calculate_bounds();
                Entry split_entry(split_bounds, split_child.release());
                node->entries.push_back(split_entry);
                
                if (node->is_full()) {
                    // Split this node too
                    new_child = split_node(node);
                    return true;
                }
            }
            return false;
        }
    }
    
    // Choose subtree with minimum enlargement
    size_t choose_subtree(const Node* node, const BoundingBox& bounds) const {
        size_t best_idx = 0;
        float best_enlargement = std::numeric_limits<float>::max();
        float best_area = std::numeric_limits<float>::max();
        
        for (size_t i = 0; i < node->entries.size(); ++i) {
            const BoundingBox& child_bounds = node->entries[i].bounds;
            BoundingBox merged = child_bounds.merge(bounds);
            
            float area = child_bounds.area();
            float enlargement = merged.area() - area;
            
            if (enlargement < best_enlargement || 
                (enlargement == best_enlargement && area < best_area)) {
                best_idx = i;
                best_enlargement = enlargement;
                best_area = area;
            }
        }
        
        return best_idx;
    }
    
    // Split node using quadratic algorithm
    NodePtr split_node(Node* node) {
        NodePtr new_node = std::make_unique<Node>(node->is_leaf);
        
        // Save all entries
        std::vector<Entry> all_entries = std::move(node->entries);
        node->entries.clear();
        
        // Find two seeds
        size_t seed1 = 0, seed2 = 1;
        float worst_waste = -1.0f;
        
        for (size_t i = 0; i < all_entries.size(); ++i) {
            for (size_t j = i + 1; j < all_entries.size(); ++j) {
                BoundingBox combined = all_entries[i].bounds.merge(all_entries[j].bounds);
                float waste = combined.area() - 
                             all_entries[i].bounds.area() - 
                             all_entries[j].bounds.area();
                
                if (waste > worst_waste) {
                    worst_waste = waste;
                    seed1 = i;
                    seed2 = j;
                }
            }
        }
        
        // Assign seeds
        node->entries.push_back(std::move(all_entries[seed1]));
        new_node->entries.push_back(std::move(all_entries[seed2]));
        
        // Mark seeds as used
        std::vector<bool> used(all_entries.size(), false);
        used[seed1] = true;
        used[seed2] = true;
        
        // Distribute remaining entries
        for (size_t i = 0; i < all_entries.size(); ++i) {
            if (used[i]) continue;
            
            // Calculate which group has less area increase
            BoundingBox bounds1 = node->calculate_bounds();
            BoundingBox bounds2 = new_node->calculate_bounds();
            
            BoundingBox new_bounds1 = bounds1.merge(all_entries[i].bounds);
            BoundingBox new_bounds2 = bounds2.merge(all_entries[i].bounds);
            
            float growth1 = new_bounds1.area() - bounds1.area();
            float growth2 = new_bounds2.area() - bounds2.area();
            
            // Assign to group with less growth (or less area if equal growth)
            if (growth1 < growth2 || 
                (growth1 == growth2 && bounds1.area() < bounds2.area())) {
                node->entries.push_back(std::move(all_entries[i]));
            } else {
                new_node->entries.push_back(std::move(all_entries[i]));
            }
            
            // Maintain minimum entries constraint
            size_t remaining = all_entries.size() - i - 1;
            if (node->entries.size() + remaining == MIN_ENTRIES) {
                // Add all remaining to node
                for (size_t j = i + 1; j < all_entries.size(); ++j) {
                    if (!used[j]) {
                        node->entries.push_back(std::move(all_entries[j]));
                    }
                }
                break;
            } else if (new_node->entries.size() + remaining == MIN_ENTRIES) {
                // Add all remaining to new_node
                for (size_t j = i + 1; j < all_entries.size(); ++j) {
                    if (!used[j]) {
                        new_node->entries.push_back(std::move(all_entries[j]));
                    }
                }
                break;
            }
        }
        
        return new_node;
    }
    
    
    
    // Recursive removal
    bool remove_recursive(Node* node, const BoundingBox& bounds, const T& value) {
        if (node->is_leaf) {
            // Find and remove from leaf
            auto it = std::find_if(node->entries.begin(), node->entries.end(),
                [&bounds, &value](const Entry& e) {
                    return e.bounds == bounds && e.value == value;
                });
            
            if (it != node->entries.end()) {
                node->entries.erase(it);
                return true;
            }
            return false;
        } else {
            // Search in children
            for (size_t i = 0; i < node->entries.size(); ++i) {
                if (node->entries[i].bounds.intersects(bounds)) {
                    if (remove_recursive(node->entries[i].child, bounds, value)) {
                        // Update bounds
                        node->entries[i].bounds = 
                            node->entries[i].child->calculate_bounds();
                        
                        // Handle underfull child
                        if (node->entries[i].child->is_underfull()) {
                            // For simplicity, we don't rebalance here
                            // A production implementation would handle this
                        }
                        
                        return true;
                    }
                }
            }
            return false;
        }
    }
    
    // Recursive query
    void query_recursive(const Node* node, const BoundingBox& bounds, 
                        QueryCallback& callback) const {
        for (const auto& entry : node->entries) {
            if (entry.bounds.intersects(bounds)) {
                if (node->is_leaf) {
                    if (!callback(entry.value)) {
                        return;  // Stop iteration
                    }
                } else if (entry.child) {  // Make sure child is not null
                    query_recursive(entry.child, bounds, callback);
                }
            }
        }
    }
    
    // Nearest neighbor search
    void nearest_neighbor_recursive(const Node* node, float x, float y,
                                   T& best_value, float& best_distance) const {
        if (node->is_leaf) {
            // Check all entries in leaf
            for (const auto& entry : node->entries) {
                float cx = (entry.bounds.min_x + entry.bounds.max_x) * 0.5f;
                float cy = (entry.bounds.min_y + entry.bounds.max_y) * 0.5f;
                
                float dx = x - cx;
                float dy = y - cy;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance < best_distance) {
                    best_distance = distance;
                    best_value = entry.value;
                }
            }
        } else {
            // Sort children by minimum distance to point
            std::vector<std::pair<float, const Entry*>> sorted_entries;
            
            for (const auto& entry : node->entries) {
                float min_dist = entry.bounds.distance_to_point(x, y);
                sorted_entries.push_back({min_dist, &entry});
            }
            
            std::sort(sorted_entries.begin(), sorted_entries.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            
            // Visit children in order of proximity
            for (const auto& [min_dist, entry] : sorted_entries) {
                if (min_dist >= best_distance) {
                    break;  // Prune remaining children
                }
                
                nearest_neighbor_recursive(entry->child, x, y, 
                                         best_value, best_distance);
            }
        }
    }
    
    // Build tree using STR algorithm
    NodePtr build_str(std::vector<std::pair<BoundingBox, T>>& items, 
                     size_t start, size_t end) {
        size_t count = end - start;
        
        if (count <= MAX_ENTRIES) {
            // Create leaf node
            auto node = std::make_unique<Node>(true);
            for (size_t i = start; i < end; ++i) {
                node->entries.emplace_back(items[i].first, items[i].second);
            }
            return node;
        }
        
        // Sort by x-coordinate
        std::sort(items.begin() + start, items.begin() + end,
            [](const auto& a, const auto& b) {
                return a.first.center_x() < b.first.center_x();
            });
        
        // Calculate number of vertical slices
        size_t slice_count = static_cast<size_t>(
            std::ceil(std::sqrt(static_cast<double>(count) / MAX_ENTRIES))
        );
        size_t slice_capacity = static_cast<size_t>(
            std::ceil(static_cast<double>(count) / slice_count)
        );
        
        // Create internal node
        auto node = std::make_unique<Node>(false);
        
        // Process each vertical slice
        for (size_t i = 0; i < slice_count; ++i) {
            size_t slice_start = start + i * slice_capacity;
            size_t slice_end = std::min(slice_start + slice_capacity, end);
            
            if (slice_start >= end) break;
            
            // Sort slice by y-coordinate
            std::sort(items.begin() + slice_start, items.begin() + slice_end,
                [](const auto& a, const auto& b) {
                    return a.first.center_y() < b.first.center_y();
                });
            
            // Build nodes for horizontal strips
            size_t strip_count = static_cast<size_t>(
                std::ceil(static_cast<double>(slice_end - slice_start) / MAX_ENTRIES)
            );
            
            for (size_t j = 0; j < strip_count; ++j) {
                size_t strip_start = slice_start + j * MAX_ENTRIES;
                size_t strip_end = std::min(strip_start + MAX_ENTRIES, slice_end);
                
                if (strip_start >= slice_end) break;
                
                NodePtr child = build_str(items, strip_start, strip_end);
                BoundingBox child_bounds = child->calculate_bounds();
                Entry entry(child_bounds, child.release());
                node->entries.push_back(entry);
            }
        }
        
        return node;
    }
    
    // Tree statistics helpers
    size_t calculate_height(const Node* node) const {
        if (!node || node->is_leaf) return 1;
        
        size_t max_height = 0;
        for (const auto& entry : node->entries) {
            max_height = std::max(max_height, calculate_height(entry.child));
        }
        return max_height + 1;
    }
    
    size_t count_nodes(const Node* node) const {
        if (!node) return 0;
        
        size_t count = 1;
        if (!node->is_leaf) {
            for (const auto& entry : node->entries) {
                count += count_nodes(entry.child);
            }
        }
        return count;
    }
};

/**
 * @brief Spatial index wrapper for shape ID lookups
 * 
 * Maps shape IDs to their spatial positions for efficient queries.
 * Integrates with SoA container for fast spatial operations.
 */
class ShapeSpatialIndex {
private:
    RTree<uint32_t> rtree_;  // Store shape IDs
    
public:
    void insert(uint32_t shape_id, const BoundingBox& bounds) {
        rtree_.insert(bounds, shape_id);
    }
    
    void remove(uint32_t shape_id, const BoundingBox& bounds) {
        rtree_.remove(bounds, shape_id);
    }
    
    std::vector<uint32_t> query_range(const BoundingBox& bounds) const {
        return rtree_.query_range(bounds);
    }
    
    std::pair<uint32_t, float> nearest_shape(float x, float y) const {
        return rtree_.nearest_neighbor(x, y);
    }
    
    void clear() {
        rtree_.clear();
    }
    
    size_t size() const {
        return rtree_.size();
    }
};

} // namespace containers
} // namespace claude_draw