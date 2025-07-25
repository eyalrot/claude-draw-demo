#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <deque>
#include <algorithm>
#include "soa_container.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Copy-on-Write container for efficient versioning
 * 
 * Provides O(1) copies until modification, then performs lazy copying.
 * Ideal for undo/redo systems, version control, and efficient snapshots.
 * 
 * Features:
 * - Shared data between copies until write
 * - Automatic deep copy on modification
 * - Thread-safe reference counting
 * - Efficient memory usage for multiple versions
 */
class CoWContainer {
private:
    // Shared data implementation
    struct SharedData {
        SoAContainer container;
        
        SharedData() = default;
        SharedData(const SharedData& other) 
            : container(other.container) {}
    };
    
    std::shared_ptr<SharedData> data_;
    
    // Ensure we have unique data before modification
    void ensure_unique() {
        if (!data_) {
            data_ = std::make_shared<SharedData>();
        } else if (data_.use_count() > 1) {
            // Need to copy
            data_ = std::make_shared<SharedData>(*data_);
        }
    }
    
public:
    CoWContainer() : data_(std::make_shared<SharedData>()) {}
    
    // Copy constructor - O(1) shallow copy
    CoWContainer(const CoWContainer& other) : data_(other.data_) {}
    
    // Move constructor
    CoWContainer(CoWContainer&& other) noexcept : data_(std::move(other.data_)) {
        other.data_ = std::make_shared<SharedData>();
    }
    
    // Copy assignment
    CoWContainer& operator=(const CoWContainer& other) {
        if (this != &other) {
            data_ = other.data_;
        }
        return *this;
    }
    
    // Move assignment
    CoWContainer& operator=(CoWContainer&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            other.data_ = std::make_shared<SharedData>();
        }
        return *this;
    }
    
    ~CoWContainer() = default;
    
    // Read operations (const) - don't trigger copy
    size_t size() const {
        return data_ ? data_->container.size() : 0;
    }
    
    bool empty() const {
        return !data_ || data_->container.empty();
    }
    
    bool contains(uint32_t id) const {
        return data_ && data_->container.contains(id);
    }
    
    BoundingBox get_bounds() const {
        return data_ ? data_->container.get_bounds() : BoundingBox(0, 0, 0, 0);
    }
    
    uint32_t ref_count() const {
        return data_ ? static_cast<uint32_t>(data_.use_count()) : 0;
    }
    
    bool is_shared() const {
        return data_ && data_.use_count() > 1;
    }
    
    // Write operations - trigger copy if shared
    uint32_t add_circle(const shapes::Circle& circle) {
        ensure_unique();
        return data_->container.add_circle(circle);
    }
    
    uint32_t add_rectangle(const shapes::Rectangle& rectangle) {
        ensure_unique();
        return data_->container.add_rectangle(rectangle);
    }
    
    uint32_t add_ellipse(const shapes::Ellipse& ellipse) {
        ensure_unique();
        return data_->container.add_ellipse(ellipse);
    }
    
    uint32_t add_line(const shapes::Line& line) {
        ensure_unique();
        return data_->container.add_line(line);
    }
    
    bool remove(uint32_t id) {
        ensure_unique();
        return data_->container.remove(id);
    }
    
    void clear() {
        ensure_unique();
        data_->container.clear();
    }
    
    // Force unique copy
    void make_unique() {
        ensure_unique();
    }
    
    // Access to underlying container (const)
    const SoAContainer& container() const {
        static const SoAContainer empty;
        return data_ ? data_->container : empty;
    }
    
    // Access to underlying container (mutable - triggers copy)
    SoAContainer& mutable_container() {
        ensure_unique();
        return data_->container;
    }
};

/**
 * @brief Version control system using CoW containers
 * 
 * Manages multiple versions efficiently using copy-on-write semantics.
 * Supports undo/redo, branching, and version limits.
 */
class CoWVersionControl {
private:
    std::deque<CoWContainer> versions_;
    size_t current_index_ = 0;
    size_t max_versions_;
    
    void trim_old_versions() {
        while (versions_.size() > max_versions_) {
            versions_.pop_front();
            if (current_index_ > 0) {
                current_index_--;
            }
        }
    }
    
public:
    explicit CoWVersionControl(size_t max_versions = 100) 
        : max_versions_(max_versions) {
        versions_.emplace_back();
    }
    
    // Current version access
    const CoWContainer& current() const {
        return versions_[current_index_];
    }
    
    CoWContainer& current_mutable() {
        // If we're modifying a past version, we need to branch
        if (current_index_ + 1 < versions_.size()) {
            // Remove any versions after current (branching)
            versions_.erase(versions_.begin() + current_index_ + 1, versions_.end());
        }
        return versions_[current_index_];
    }
    
    // Version management
    void checkpoint() {
        // Remove any versions after current (branching)
        versions_.erase(versions_.begin() + current_index_ + 1, versions_.end());
        
        // Add new version (CoW copy)
        versions_.push_back(versions_[current_index_]);
        current_index_++;
        
        // Trim old versions if needed
        trim_old_versions();
    }
    
    bool undo() {
        if (current_index_ > 0) {
            current_index_--;
            return true;
        }
        return false;
    }
    
    bool redo() {
        if (current_index_ + 1 < versions_.size()) {
            current_index_++;
            return true;
        }
        return false;
    }
    
    void go_to_version(size_t index) {
        if (index < versions_.size()) {
            current_index_ = index;
        }
    }
    
    void clear_history() {
        CoWContainer current_copy = versions_[current_index_];
        versions_.clear();
        versions_.push_back(std::move(current_copy));
        current_index_ = 0;
    }
    
    // Version info
    size_t version_count() const {
        return versions_.size();
    }
    
    size_t current_version_index() const {
        return current_index_;
    }
    
    bool can_undo() const {
        return current_index_ > 0;
    }
    
    bool can_redo() const {
        return current_index_ + 1 < versions_.size();
    }
};

} // namespace containers
} // namespace claude_draw