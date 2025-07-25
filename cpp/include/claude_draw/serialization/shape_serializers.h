#pragma once

#include "claude_draw/serialization/binary_serializer.h"
#include "claude_draw/serialization/binary_deserializer.h"
#include "claude_draw/shapes/circle_optimized.h"
#include "claude_draw/shapes/rectangle_optimized.h"
#include "claude_draw/shapes/line_optimized.h"
#include "claude_draw/shapes/ellipse_optimized.h"
#include "claude_draw/containers/group.h"
#include "claude_draw/containers/layer.h"
#include "claude_draw/containers/drawing.h"

namespace claude_draw {
namespace serialization {

/**
 * @brief Serialization helpers for core types
 */

// Point2D serialization
inline void serialize(BinaryWriter& writer, const Point2D& point) {
    writer.write_float(point.x);
    writer.write_float(point.y);
}

inline Point2D deserialize_point(BinaryReader& reader) {
    float x = reader.read_float();
    float y = reader.read_float();
    return Point2D(x, y);
}

// Color serialization
inline void serialize(BinaryWriter& writer, const Color& color) {
    writer.write_uint32(color.rgba);
}

inline Color deserialize_color(BinaryReader& reader) {
    uint32_t rgba = reader.read_uint32();
    return Color(rgba);
}

// Transform2D serialization
inline void serialize(BinaryWriter& writer, const Transform2D& transform) {
    // Write matrix elements
    writer.write_float(transform(0, 0));
    writer.write_float(transform(0, 1));
    writer.write_float(transform(0, 2));
    writer.write_float(transform(1, 0));
    writer.write_float(transform(1, 1));
    writer.write_float(transform(1, 2));
}

inline Transform2D deserialize_transform(BinaryReader& reader) {
    float m00 = reader.read_float();
    float m01 = reader.read_float();
    float m02 = reader.read_float();
    float m10 = reader.read_float();
    float m11 = reader.read_float();
    float m12 = reader.read_float();
    
    return Transform2D(m00, m01, m02, m10, m11, m12);
}

// BoundingBox serialization
inline void serialize(BinaryWriter& writer, const BoundingBox& bbox) {
    writer.write_float(bbox.min_x);
    writer.write_float(bbox.min_y);
    writer.write_float(bbox.max_x);
    writer.write_float(bbox.max_y);
}

inline BoundingBox deserialize_bbox(BinaryReader& reader) {
    float min_x = reader.read_float();
    float min_y = reader.read_float();
    float max_x = reader.read_float();
    float max_y = reader.read_float();
    return BoundingBox(min_x, min_y, max_x, max_y);
}

/**
 * @brief Shape serialization functions
 */

// Circle serialization
inline void serialize(BinaryWriter& writer, const shapes::Circle& circle) {
    // Write base shape data
    writer.write_uint32(circle.get_id());
    
    // Write circle-specific data
    serialize(writer, circle.get_center());
    writer.write_float(circle.get_radius());
    serialize(writer, circle.get_fill_color());
    serialize(writer, circle.get_stroke_color());
    writer.write_float(circle.get_stroke_width());
}

inline std::unique_ptr<shapes::Circle> deserialize_circle(BinaryReader& reader) {
    // Read base shape data
    uint32_t id = reader.read_uint32();
    
    // Read circle-specific data
    Point2D center = deserialize_point(reader);
    float radius = reader.read_float();
    Color fill = deserialize_color(reader);
    Color stroke = deserialize_color(reader);
    float stroke_width = reader.read_float();
    
    auto circle = std::make_unique<shapes::Circle>(center, radius);
    circle->set_id(id);
    circle->set_fill_color(fill);
    circle->set_stroke_color(stroke);
    circle->set_stroke_width(stroke_width);
    
    return circle;
}

// Rectangle serialization
inline void serialize(BinaryWriter& writer, const shapes::Rectangle& rect) {
    // Write base shape data
    writer.write_uint32(rect.get_id());
    
    // Write rectangle-specific data
    writer.write_float(rect.get_x());
    writer.write_float(rect.get_y());
    writer.write_float(rect.get_width());
    writer.write_float(rect.get_height());
    serialize(writer, rect.get_fill_color());
    serialize(writer, rect.get_stroke_color());
    writer.write_float(rect.get_stroke_width());
}

inline std::unique_ptr<shapes::Rectangle> deserialize_rectangle(BinaryReader& reader) {
    // Read base shape data
    uint32_t id = reader.read_uint32();
    
    // Read rectangle-specific data
    float x = reader.read_float();
    float y = reader.read_float();
    float width = reader.read_float();
    float height = reader.read_float();
    Color fill = deserialize_color(reader);
    Color stroke = deserialize_color(reader);
    float stroke_width = reader.read_float();
    
    auto rect = std::make_unique<shapes::Rectangle>(x, y, width, height);
    rect->set_id(id);
    rect->set_fill_color(fill);
    rect->set_stroke_color(stroke);
    rect->set_stroke_width(stroke_width);
    
    return rect;
}

// Line serialization
inline void serialize(BinaryWriter& writer, const shapes::Line& line) {
    // Write base shape data
    writer.write_uint32(line.get_id());
    
    // Write line-specific data
    serialize(writer, line.get_start());
    serialize(writer, line.get_end());
    serialize(writer, line.get_stroke_color());
    writer.write_float(line.get_stroke_width());
}

inline std::unique_ptr<shapes::Line> deserialize_line(BinaryReader& reader) {
    // Read base shape data
    uint32_t id = reader.read_uint32();
    
    // Read line-specific data
    Point2D start = deserialize_point(reader);
    Point2D end = deserialize_point(reader);
    Color stroke = deserialize_color(reader);
    float stroke_width = reader.read_float();
    
    auto line = std::make_unique<shapes::Line>(start, end);
    line->set_id(id);
    line->set_stroke_color(stroke);
    line->set_stroke_width(stroke_width);
    
    return line;
}

// Ellipse serialization
inline void serialize(BinaryWriter& writer, const shapes::Ellipse& ellipse) {
    // Write base shape data
    writer.write_uint32(ellipse.get_id());
    
    // Write ellipse-specific data
    serialize(writer, ellipse.get_center());
    writer.write_float(ellipse.get_radius_x());
    writer.write_float(ellipse.get_radius_y());
    serialize(writer, ellipse.get_fill_color());
    serialize(writer, ellipse.get_stroke_color());
    writer.write_float(ellipse.get_stroke_width());
}

inline std::unique_ptr<shapes::Ellipse> deserialize_ellipse(BinaryReader& reader) {
    // Read base shape data
    uint32_t id = reader.read_uint32();
    
    // Read ellipse-specific data
    Point2D center = deserialize_point(reader);
    float rx = reader.read_float();
    float ry = reader.read_float();
    Color fill = deserialize_color(reader);
    Color stroke = deserialize_color(reader);
    float stroke_width = reader.read_float();
    
    auto ellipse = std::make_unique<shapes::Ellipse>(center, rx, ry);
    ellipse->set_id(id);
    ellipse->set_fill_color(fill);
    ellipse->set_stroke_color(stroke);
    ellipse->set_stroke_width(stroke_width);
    
    return ellipse;
}

/**
 * @brief Container serialization - forward declarations
 */
class ShapeSerializer {
public:
    static void serialize_shape(BinaryWriter& writer, const shapes::ShapeBase* shape);
    static std::unique_ptr<shapes::ShapeBase> deserialize_shape(BinaryReader& reader, TypeId type);
};

// Group serialization
inline void serialize(BinaryWriter& writer, const containers::Group& group) {
    // Write base data
    writer.write_uint32(group.get_id());
    
    // Write shapes array
    const auto& shapes = group.get_shapes();
    writer.write_uint32(static_cast<uint32_t>(shapes.size()));
    
    for (const auto& shape : shapes) {
        // Write type ID
        TypeId type = TypeId::None;
        if (dynamic_cast<const shapes::Circle*>(shape.get())) {
            type = TypeId::Circle;
        } else if (dynamic_cast<const shapes::Rectangle*>(shape.get())) {
            type = TypeId::Rectangle;
        } else if (dynamic_cast<const shapes::Line*>(shape.get())) {
            type = TypeId::Line;
        } else if (dynamic_cast<const shapes::Ellipse*>(shape.get())) {
            type = TypeId::Ellipse;
        } else if (dynamic_cast<const containers::Group*>(shape.get())) {
            type = TypeId::Group;
        }
        
        writer.write_uint8(static_cast<uint8_t>(type));
        ShapeSerializer::serialize_shape(writer, shape.get());
    }
}

inline std::unique_ptr<containers::Group> deserialize_group(BinaryReader& reader) {
    // Read base data
    uint32_t id = reader.read_uint32();
    
    auto group = std::make_unique<containers::Group>();
    group->set_id(id);
    
    // Read shapes array
    uint32_t shape_count = reader.read_uint32();
    for (uint32_t i = 0; i < shape_count; ++i) {
        TypeId type = static_cast<TypeId>(reader.read_uint8());
        auto shape = ShapeSerializer::deserialize_shape(reader, type);
        if (shape) {
            group->add_shape(std::move(shape));
        }
    }
    
    return group;
}

// Layer serialization
inline void serialize(BinaryWriter& writer, const containers::Layer& layer) {
    // Write layer data
    writer.write_uint32(layer.get_id());
    writer.write_string(layer.get_name());
    writer.write_uint8(layer.is_visible() ? 1 : 0);
    writer.write_uint8(layer.is_locked() ? 1 : 0);
    writer.write_float(layer.get_opacity());
    
    // Write shapes
    const auto& shapes = layer.get_shapes();
    writer.write_uint32(static_cast<uint32_t>(shapes.size()));
    
    for (const auto& shape : shapes) {
        TypeId type = TypeId::None;
        if (dynamic_cast<const shapes::Circle*>(shape.get())) {
            type = TypeId::Circle;
        } else if (dynamic_cast<const shapes::Rectangle*>(shape.get())) {
            type = TypeId::Rectangle;
        } else if (dynamic_cast<const shapes::Line*>(shape.get())) {
            type = TypeId::Line;
        } else if (dynamic_cast<const shapes::Ellipse*>(shape.get())) {
            type = TypeId::Ellipse;
        } else if (dynamic_cast<const containers::Group*>(shape.get())) {
            type = TypeId::Group;
        }
        
        writer.write_uint8(static_cast<uint8_t>(type));
        ShapeSerializer::serialize_shape(writer, shape.get());
    }
}

inline std::unique_ptr<containers::Layer> deserialize_layer(BinaryReader& reader) {
    // Read layer data
    uint32_t id = reader.read_uint32();
    std::string name = reader.read_string();
    bool visible = reader.read_uint8() != 0;
    bool locked = reader.read_uint8() != 0;
    float opacity = reader.read_float();
    
    auto layer = std::make_unique<containers::Layer>(name);
    layer->set_id(id);
    layer->set_visible(visible);
    layer->set_locked(locked);
    layer->set_opacity(opacity);
    
    // Read shapes
    uint32_t shape_count = reader.read_uint32();
    for (uint32_t i = 0; i < shape_count; ++i) {
        TypeId type = static_cast<TypeId>(reader.read_uint8());
        auto shape = ShapeSerializer::deserialize_shape(reader, type);
        if (shape) {
            layer->add_shape(std::move(shape));
        }
    }
    
    return layer;
}

// Drawing serialization
inline void serialize(BinaryWriter& writer, const containers::Drawing& drawing) {
    // Write drawing metadata
    writer.write_float(drawing.get_width());
    writer.write_float(drawing.get_height());
    writer.write_string(drawing.get_title());
    writer.write_string(drawing.get_description());
    
    // Write background color
    serialize(writer, drawing.get_background_color());
    
    // Write layers
    const auto& layers = drawing.get_layers();
    writer.write_uint32(static_cast<uint32_t>(layers.size()));
    
    for (const auto& layer : layers) {
        serialize(writer, *layer);
    }
}

inline std::unique_ptr<containers::Drawing> deserialize_drawing(BinaryReader& reader) {
    // Read drawing metadata
    float width = reader.read_float();
    float height = reader.read_float();
    std::string title = reader.read_string();
    std::string description = reader.read_string();
    
    auto drawing = std::make_unique<containers::Drawing>(width, height, title);
    drawing->set_description(description);
    
    // Read background color
    Color bg = deserialize_color(reader);
    drawing->set_background_color(bg);
    
    // Read layers
    uint32_t layer_count = reader.read_uint32();
    for (uint32_t i = 0; i < layer_count; ++i) {
        auto layer = deserialize_layer(reader);
        if (layer) {
            drawing->add_layer(std::move(layer));
        }
    }
    
    return drawing;
}

// Implementation of ShapeSerializer
inline void ShapeSerializer::serialize_shape(BinaryWriter& writer, const shapes::ShapeBase* shape) {
    if (auto circle = dynamic_cast<const shapes::Circle*>(shape)) {
        serialize(writer, *circle);
    } else if (auto rect = dynamic_cast<const shapes::Rectangle*>(shape)) {
        serialize(writer, *rect);
    } else if (auto line = dynamic_cast<const shapes::Line*>(shape)) {
        serialize(writer, *line);
    } else if (auto ellipse = dynamic_cast<const shapes::Ellipse*>(shape)) {
        serialize(writer, *ellipse);
    } else if (auto group = dynamic_cast<const containers::Group*>(shape)) {
        serialize(writer, *group);
    }
}

inline std::unique_ptr<shapes::ShapeBase> ShapeSerializer::deserialize_shape(BinaryReader& reader, TypeId type) {
    switch (type) {
        case TypeId::Circle:
            return deserialize_circle(reader);
        case TypeId::Rectangle:
            return deserialize_rectangle(reader);
        case TypeId::Line:
            return deserialize_line(reader);
        case TypeId::Ellipse:
            return deserialize_ellipse(reader);
        case TypeId::Group:
            return deserialize_group(reader);
        default:
            return nullptr;
    }
}

} // namespace serialization
} // namespace claude_draw