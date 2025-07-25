#pragma once

#include <vector>
#include <memory>
#include <string>
#include "claude_draw/shapes/shape_base.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Layer container for organizing shapes
 */
class Layer {
private:
    uint32_t id_ = 0;
    std::string name_;
    std::vector<std::unique_ptr<shapes::ShapeBase>> shapes_;
    bool visible_ = true;
    bool locked_ = false;
    float opacity_ = 1.0f;

public:
    Layer() : name_("Untitled Layer") {}
    explicit Layer(const std::string& name) : name_(name) {}
    
    // Property accessors
    uint32_t get_id() const { return id_; }
    void set_id(uint32_t id) { id_ = id; }
    
    const std::string& get_name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }
    
    bool is_visible() const { return visible_; }
    void set_visible(bool visible) { visible_ = visible; }
    
    bool is_locked() const { return locked_; }
    void set_locked(bool locked) { locked_ = locked; }
    
    float get_opacity() const { return opacity_; }
    void set_opacity(float opacity) { opacity_ = std::clamp(opacity, 0.0f, 1.0f); }
    
    // Shape management
    void add_shape(std::unique_ptr<shapes::ShapeBase> shape) {
        if (!locked_) {
            shapes_.push_back(std::move(shape));
        }
    }
    
    const std::vector<std::unique_ptr<shapes::ShapeBase>>& get_shapes() const {
        return shapes_;
    }
    
    std::vector<std::unique_ptr<shapes::ShapeBase>>& get_shapes() {
        return shapes_;
    }
    
    void clear() {
        if (!locked_) {
            shapes_.clear();
        }
    }
    
    size_t size() const {
        return shapes_.size();
    }
    
    // Compute layer bounds
    BoundingBox get_bounds() const {
        if (shapes_.empty()) {
            return BoundingBox();
        }
        
        BoundingBox bounds = shapes_[0]->get_bounds();
        for (size_t i = 1; i < shapes_.size(); ++i) {
            bounds = bounds.union_with(shapes_[i]->get_bounds());
        }
        return bounds;
    }
};

} // namespace containers
} // namespace claude_draw