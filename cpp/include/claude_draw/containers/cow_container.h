#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include "soa_container.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Copy-on-Write wrapper for SoAContainer
 * 
 * This container provides efficient cloning and modification by sharing
 * underlying data until a write operation occurs. This is particularly
 * useful for:
 * - Undo/redo operations
 * - Version control of drawings
 * - Efficient temporary modifications
 * - Multi-threaded read access with occasional writes
 * 
 * Features:
 * - O(1) copy operations
 * - Lazy copying on modification
 * - Thread-safe read operations
 * - Reference counting for automatic cleanup
 */
class CoWContainer {
public:
    /**
     * @brief Shared data holder with reference counting
     */
    struct SharedData {
        std::unique_ptr<SoAContainer> container;
        std::atomic<size_t> ref_count{1};
        mutable std::shared_mutex read_write_lock;
        
        SharedData() : container(std::make_unique<SoAContainer>()) {}
        
        // Deep copy constructor
        SharedData(const SharedData& other) 
            : container(std::make_unique<SoAContainer>())
            , ref_count(1) {
            // We need to manually copy the container's contents
            // since SoAContainer is not copyable due to mutex/atomic members
            std::shared_lock lock(other.read_write_lock);
            copy_container_contents(*other.container, *container);
        }
        
    private:
        // Helper to copy container contents
        static void copy_container_contents(const SoAContainer& from, SoAContainer& to) {
            // Clear destination
            to.clear();
            
            // Reserve capacity
            to.reserve(from.size());
            
            // Copy all shapes by iterating through them
            // Note: const_cast is safe here because we're only reading
            const_cast<SoAContainer&>(from).for_each_sorted([&to](const auto& shape) {
                using ShapeType = std::decay_t<decltype(shape)>;
                if constexpr (std::is_same_v<ShapeType, shapes::Circle>) {
                    to.add_circle(shape);
                } else if constexpr (std::is_same_v<ShapeType, shapes::Rectangle>) {
                    to.add_rectangle(shape);
                } else if constexpr (std::is_same_v<ShapeType, shapes::Ellipse>) {
                    to.add_ellipse(shape);
                } else if constexpr (std::is_same_v<ShapeType, shapes::Line>) {
                    to.add_line(shape);
                }
            });
        }
    };
    
    CoWContainer() 
        : data_(std::make_shared<SharedData>()) {}
    
    /**
     * @brief Copy constructor - O(1) operation
     * 
     * Creates a new container that shares data with the original
     * until one of them is modified.
     */
    CoWContainer(const CoWContainer& other) noexcept
        : data_(other.data_) {
        data_->ref_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    /**
     * @brief Move constructor
     */
    CoWContainer(CoWContainer&& other) noexcept
        : data_(std::move(other.data_)) {
        other.data_ = std::make_shared<SharedData>();
    }
    
    /**
     * @brief Copy assignment - O(1) operation
     */
    CoWContainer& operator=(const CoWContainer& other) noexcept {
        if (this != &other) {
            // Decrement old reference
            if (data_ && data_->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                // We were the last reference, data will be cleaned up automatically
            }
            
            // Share new data
            data_ = other.data_;
            data_->ref_count.fetch_add(1, std::memory_order_relaxed);
        }
        return *this;
    }
    
    /**
     * @brief Move assignment
     */
    CoWContainer& operator=(CoWContainer&& other) noexcept {
        if (this != &other) {
            // Decrement old reference
            if (data_ && data_->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                // We were the last reference
            }
            
            data_ = std::move(other.data_);
            other.data_ = std::make_shared<SharedData>();
        }
        return *this;
    }
    
    ~CoWContainer() {
        if (data_ && data_->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // We were the last reference, shared_ptr will clean up
        }
    }
    
    // Read operations (don't trigger copy)
    size_t size() const {
        std::shared_lock lock(data_->read_write_lock);
        return data_->container->size();
    }
    
    size_t count(shapes::ShapeType type) const {
        std::shared_lock lock(data_->read_write_lock);
        return data_->container->count(type);
    }
    
    bool contains(uint32_t id) const {
        std::shared_lock lock(data_->read_write_lock);
        return data_->container->contains(id);
    }
    
    BoundingBox get_bounds() const {
        std::shared_lock lock(data_->read_write_lock);
        return data_->container->get_bounds();
    }
    
    /**
     * @brief Get shape by ID (read-only)
     * 
     * Returns nullptr if shape doesn't exist or type doesn't match.
     */
    template<typename ShapeT>
    const ShapeT* get(uint32_t id) const {
        std::shared_lock lock(data_->read_write_lock);
        return data_->container->template get<ShapeT>(id);
    }
    
    /**
     * @brief Iterate shapes of a specific type (read-only)
     */
    template<typename ShapeT, typename Func>
    void for_each_type(Func func) const {
        std::shared_lock lock(data_->read_write_lock);
        const_cast<const SoAContainer*>(data_->container.get())->template for_each_type<ShapeT>(
            [&func](const ShapeT& shape) { func(shape); });
    }
    
    /**
     * @brief Iterate all shapes in z-order (read-only)
     */
    template<typename Func>
    void for_each_sorted(Func func) const {
        std::shared_lock lock(data_->read_write_lock);
        const_cast<const SoAContainer*>(data_->container.get())->for_each_sorted(func);
    }
    
    // Write operations (trigger copy-on-write)
    
    /**
     * @brief Add a circle (triggers CoW if shared)
     */
    uint32_t add_circle(const shapes::Circle& circle) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        return data_->container->add_circle(circle);
    }
    
    /**
     * @brief Add a rectangle (triggers CoW if shared)
     */
    uint32_t add_rectangle(const shapes::Rectangle& rect) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        return data_->container->add_rectangle(rect);
    }
    
    /**
     * @brief Add an ellipse (triggers CoW if shared)
     */
    uint32_t add_ellipse(const shapes::Ellipse& ellipse) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        return data_->container->add_ellipse(ellipse);
    }
    
    /**
     * @brief Add a line (triggers CoW if shared)
     */
    uint32_t add_line(const shapes::Line& line) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        return data_->container->add_line(line);
    }
    
    /**
     * @brief Remove shape by ID (triggers CoW if shared)
     */
    bool remove(uint32_t id) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        return data_->container->remove(id);
    }
    
    /**
     * @brief Set visibility (triggers CoW if shared)
     */
    bool set_visible(uint32_t id, bool visible) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        return data_->container->set_visible(id, visible);
    }
    
    /**
     * @brief Set z-index (triggers CoW if shared)
     */
    bool set_z_index(uint32_t id, uint16_t z_index) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        return data_->container->set_z_index(id, z_index);
    }
    
    /**
     * @brief Clear all shapes (triggers CoW if shared)
     */
    void clear() {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        data_->container->clear();
    }
    
    /**
     * @brief Reserve capacity (triggers CoW if shared)
     */
    void reserve(size_t capacity) {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        data_->container->reserve(capacity);
    }
    
    /**
     * @brief Compact arrays (triggers CoW if shared)
     */
    void compact() {
        ensure_unique();
        std::unique_lock lock(data_->read_write_lock);
        data_->container->compact();
    }
    
    /**
     * @brief Check if this container is sharing data
     */
    bool is_shared() const {
        return data_->ref_count.load(std::memory_order_acquire) > 1;
    }
    
    /**
     * @brief Get the current reference count
     */
    size_t ref_count() const {
        return data_->ref_count.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Force a deep copy (useful before a series of modifications)
     */
    void make_unique() {
        ensure_unique();
    }
    
    /**
     * @brief Get direct access to the underlying container (const)
     * 
     * Use with caution - bypasses CoW semantics.
     */
    const SoAContainer& get_container() const {
        return *data_->container;
    }
    
private:
    std::shared_ptr<SharedData> data_;
    
    /**
     * @brief Ensure we have a unique copy of the data
     * 
     * This is called before any write operation to implement
     * copy-on-write semantics.
     */
    void ensure_unique() {
        if (data_->ref_count.load(std::memory_order_acquire) > 1) {
            // We're sharing data, need to make a copy
            auto new_data = std::make_shared<SharedData>(*data_);
            
            // Decrement old reference count
            data_->ref_count.fetch_sub(1, std::memory_order_acq_rel);
            
            // Use the new copy
            data_ = new_data;
        }
    }
};

/**
 * @brief Version control system for CoW containers
 * 
 * Provides undo/redo functionality and version management.
 */
class CoWVersionControl {
public:
    static constexpr size_t DEFAULT_MAX_VERSIONS = 100;
    
    CoWVersionControl(size_t max_versions = DEFAULT_MAX_VERSIONS)
        : max_versions_(max_versions) {
        // Start with an empty container
        versions_.emplace_back();
        current_version_ = 0;
    }
    
    /**
     * @brief Get the current container
     */
    const CoWContainer& current() const {
        return versions_[current_version_];
    }
    
    /**
     * @brief Get a mutable reference to current container
     * 
     * Returns the current version for modification. The version tracking
     * happens at checkpoint() time, not on each modification.
     */
    CoWContainer& current_mutable() {
        // If we're not at the latest version, we need to branch
        if (current_version_ < versions_.size() - 1) {
            // Remove all versions after current
            versions_.erase(versions_.begin() + current_version_ + 1, versions_.end());
        }
        
        return versions_[current_version_];
    }
    
    /**
     * @brief Save the current state as a new version
     */
    void checkpoint() {
        if (current_version_ < versions_.size() - 1) {
            versions_.erase(versions_.begin() + current_version_ + 1, versions_.end());
        }
        
        versions_.push_back(versions_[current_version_]);
        current_version_++;
        
        if (versions_.size() > max_versions_) {
            versions_.erase(versions_.begin());
            current_version_--;
        }
    }
    
    /**
     * @brief Undo to previous version
     */
    bool undo() {
        if (current_version_ > 0) {
            current_version_--;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Redo to next version
     */
    bool redo() {
        if (current_version_ < versions_.size() - 1) {
            current_version_++;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Get number of versions
     */
    size_t version_count() const {
        return versions_.size();
    }
    
    /**
     * @brief Get current version index
     */
    size_t current_version_index() const {
        return current_version_;
    }
    
    /**
     * @brief Jump to specific version
     */
    bool go_to_version(size_t version) {
        if (version < versions_.size()) {
            current_version_ = version;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Clear all versions except current
     */
    void clear_history() {
        CoWContainer current = versions_[current_version_];
        versions_.clear();
        versions_.push_back(std::move(current));
        current_version_ = 0;
    }
    
private:
    std::vector<CoWContainer> versions_;
    size_t current_version_;
    size_t max_versions_;
};

} // namespace containers
} // namespace claude_draw