#pragma once

#include "claude_draw/serialization/binary_serializer.h"
#include "claude_draw/serialization/binary_deserializer.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"

namespace claude_draw {
namespace serialization {

/**
 * @brief Simple serialization helpers for core types
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

} // namespace serialization
} // namespace claude_draw