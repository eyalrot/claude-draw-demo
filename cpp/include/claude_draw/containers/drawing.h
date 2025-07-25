#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "claude_draw/containers/layer.h"
#include "claude_draw/core/color.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Drawing container - top-level container for a complete drawing
 */
class Drawing {
private:
    float width_;
    float height_;
    std::string title_;
    std::string description_;
    Color background_color_;
    std::vector<std::unique_ptr<Layer>> layers_;

public:
    Drawing(float width, float height, const std::string& title = "Untitled")
        : width_(width)
        , height_(height)
        , title_(title)
        , description_("")
        , background_color_(uint8_t(255), uint8_t(255), uint8_t(255), uint8_t(255)) // Default white background
    {
        // Create default layer
        add_layer(std::make_unique<Layer>("Layer 1"));
    }
    
    // Property accessors
    float get_width() const { return width_; }
    void set_width(float width) { width_ = width; }
    
    float get_height() const { return height_; }
    void set_height(float height) { height_ = height; }
    
    const std::string& get_title() const { return title_; }
    void set_title(const std::string& title) { title_ = title; }
    
    const std::string& get_description() const { return description_; }
    void set_description(const std::string& desc) { description_ = desc; }
    
    const Color& get_background_color() const { return background_color_; }
    void set_background_color(const Color& color) { background_color_ = color; }
    
    // Layer management
    void add_layer(std::unique_ptr<Layer> layer) {
        layers_.push_back(std::move(layer));
    }
    
    void insert_layer(size_t index, std::unique_ptr<Layer> layer) {
        if (index <= layers_.size()) {
            layers_.insert(layers_.begin() + index, std::move(layer));
        }
    }
    
    void remove_layer(size_t index) {
        if (index < layers_.size()) {
            layers_.erase(layers_.begin() + index);
        }
    }
    
    Layer* get_layer(size_t index) {
        return (index < layers_.size()) ? layers_[index].get() : nullptr;
    }
    
    const Layer* get_layer(size_t index) const {
        return (index < layers_.size()) ? layers_[index].get() : nullptr;
    }
    
    const std::vector<std::unique_ptr<Layer>>& get_layers() const {
        return layers_;
    }
    
    std::vector<std::unique_ptr<Layer>>& get_layers() {
        return layers_;
    }
    
    size_t layer_count() const {
        return layers_.size();
    }
    
    // Get drawing bounds
    BoundingBox get_bounds() const {
        return BoundingBox(0, 0, width_, height_);
    }
    
    // Get content bounds (union of all layer bounds)
    BoundingBox get_content_bounds() const {
        if (layers_.empty()) {
            return BoundingBox();
        }
        
        BoundingBox bounds;
        bool first = true;
        
        for (const auto& layer : layers_) {
            if (layer->is_visible() && layer->size() > 0) {
                if (first) {
                    bounds = layer->get_bounds();
                    first = false;
                } else {
                    bounds = bounds.union_with(layer->get_bounds());
                }
            }
        }
        
        return bounds;
    }
};

} // namespace containers
} // namespace claude_draw