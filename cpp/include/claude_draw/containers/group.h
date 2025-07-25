#pragma once

#include <vector>
#include <memory>
#include "claude_draw/shapes/shape_base.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Simple group container for shapes
 */
class Group : public shapes::ShapeBase {
private:
    std::vector<std::unique_ptr<shapes::ShapeBase>> shapes_;

public:
    Group() : ShapeBase(shapes::ShapeType::Circle) {} // Using Circle as placeholder
    
    // Shape management
    void add_shape(std::unique_ptr<shapes::ShapeBase> shape) {
        shapes_.push_back(std::move(shape));
        bounds_valid_ = false;
    }
    
    const std::vector<std::unique_ptr<shapes::ShapeBase>>& get_shapes() const {
        return shapes_;
    }
    
    std::vector<std::unique_ptr<shapes::ShapeBase>>& get_shapes() {
        return shapes_;
    }
    
    void clear() {
        shapes_.clear();
        bounds_valid_ = false;
    }
    
    size_t size() const {
        return shapes_.size();
    }
    
    // ShapeBase overrides
    BoundingBox compute_bounds() const override {
        if (shapes_.empty()) {
            return BoundingBox();
        }
        
        BoundingBox bounds = shapes_[0]->get_bounds();
        for (size_t i = 1; i < shapes_.size(); ++i) {
            bounds = bounds.union_with(shapes_[i]->get_bounds());
        }
        return bounds;
    }
    
    bool contains_point(const Point2D& point) const override {
        for (const auto& shape : shapes_) {
            if (shape->contains_point(point)) {
                return true;
            }
        }
        return false;
    }
};

} // namespace containers
} // namespace claude_draw