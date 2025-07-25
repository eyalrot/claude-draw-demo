#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <sstream>

#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"

namespace py = pybind11;
using namespace claude_draw;

// Custom type casters for zero-copy conversions
namespace pybind11 { namespace detail {

// Point2D type caster - enables efficient conversion between numpy arrays and Point2D
template <>
struct type_caster<Point2D> {
public:
    PYBIND11_TYPE_CASTER(Point2D, _("Point2D"));
    
    // Python -> C++ conversion
    bool load(handle src, bool) {
        // Try to convert from tuple/list of 2 floats
        if (py::isinstance<py::tuple>(src) || py::isinstance<py::list>(src)) {
            auto seq = py::reinterpret_borrow<py::sequence>(src);
            if (seq.size() != 2) return false;
            
            try {
                value.x = seq[0].cast<float>();
                value.y = seq[1].cast<float>();
                return true;
            } catch (...) {
                return false;
            }
        }
        
        // Try to convert from numpy array
        if (py::isinstance<py::array_t<float>>(src)) {
            auto arr = py::cast<py::array_t<float>>(src);
            if (arr.ndim() != 1 || arr.shape(0) != 2) return false;
            
            auto ptr = static_cast<const float*>(arr.data());
            value.x = ptr[0];
            value.y = ptr[1];
            return true;
        }
        
        // Try to convert from object with x, y attributes
        if (py::hasattr(src, "x") && py::hasattr(src, "y")) {
            try {
                value.x = src.attr("x").cast<float>();
                value.y = src.attr("y").cast<float>();
                return true;
            } catch (...) {
                return false;
            }
        }
        
        return false;
    }
    
    // C++ -> Python conversion
    static handle cast(const Point2D& src, return_value_policy, handle) {
        // Return as a simple tuple for efficiency
        return py::make_tuple(src.x, src.y).release();
    }
};

// Color type caster - enables efficient conversion between integers and Color
template <>
struct type_caster<Color> {
public:
    PYBIND11_TYPE_CASTER(Color, _("Color"));
    
    // Python -> C++ conversion
    bool load(handle src, bool) {
        // Try to convert from integer (packed RGBA)
        if (py::isinstance<py::int_>(src)) {
            value.rgba = src.cast<uint32_t>();
            return true;
        }
        
        // Try to convert from tuple/list of 3 or 4 values
        if (py::isinstance<py::tuple>(src) || py::isinstance<py::list>(src)) {
            auto seq = py::reinterpret_borrow<py::sequence>(src);
            if (seq.size() < 3 || seq.size() > 4) return false;
            
            try {
                // Check if values are floats (0-1) or ints (0-255)
                auto first = seq[0];
                if (py::isinstance<py::float_>(first)) {
                    // Float values
                    float r = seq[0].cast<float>();
                    float g = seq[1].cast<float>();
                    float b = seq[2].cast<float>();
                    float a = (seq.size() == 4) ? seq[3].cast<float>() : 1.0f;
                    value = Color(r, g, b, a);
                } else {
                    // Integer values
                    uint8_t r = seq[0].cast<uint8_t>();
                    uint8_t g = seq[1].cast<uint8_t>();
                    uint8_t b = seq[2].cast<uint8_t>();
                    uint8_t a = (seq.size() == 4) ? seq[3].cast<uint8_t>() : 255;
                    value = Color(r, g, b, a);
                }
                return true;
            } catch (...) {
                return false;
            }
        }
        
        // Try to convert from object with r, g, b, a attributes
        if (py::hasattr(src, "r") && py::hasattr(src, "g") && py::hasattr(src, "b")) {
            try {
                // Check if attributes are floats or ints
                auto r_attr = src.attr("r");
                if (py::isinstance<py::float_>(r_attr)) {
                    // Float attributes
                    float r = r_attr.cast<float>();
                    float g = src.attr("g").cast<float>();
                    float b = src.attr("b").cast<float>();
                    float a = py::hasattr(src, "a") ? src.attr("a").cast<float>() : 1.0f;
                    value = Color(r, g, b, a);
                } else {
                    // Integer attributes
                    uint8_t r = r_attr.cast<uint8_t>();
                    uint8_t g = src.attr("g").cast<uint8_t>();
                    uint8_t b = src.attr("b").cast<uint8_t>();
                    uint8_t a = py::hasattr(src, "a") ? src.attr("a").cast<uint8_t>() : 255;
                    value = Color(r, g, b, a);
                }
                return true;
            } catch (...) {
                return false;
            }
        }
        
        return false;
    }
    
    // C++ -> Python conversion
    static handle cast(const Color& src, return_value_policy, handle) {
        // Return as integer for efficiency
        return py::int_(src.rgba).release();
    }
};

}} // namespace pybind11::detail

// Helper function to create numpy array views of C++ data
template<typename T>
py::array_t<float> create_numpy_view(T* data, size_t count, size_t stride = sizeof(T)) {
    // Create numpy array that references the C++ memory (no copy)
    return py::array_t<float>(
        {count, size_t(2)},  // Shape: (count, 2) for Point2D
        {stride, sizeof(float)},  // Strides
        reinterpret_cast<float*>(data),
        py::cast(data)  // Keep reference to prevent deallocation
    );
}

void bind_core_types(py::module& m) {
    // Point2D binding
    py::class_<Point2D>(m, "Point2D", py::buffer_protocol())
        .def(py::init<>())
        .def(py::init<float, float>(), py::arg("x"), py::arg("y"))
        .def(py::init<const Point2D&>())
        .def_readwrite("x", &Point2D::x)
        .def_readwrite("y", &Point2D::y)
        
        // Arithmetic operators
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * float())
        .def(float() * py::self)
        .def(py::self / float())
        .def(-py::self)
        .def(py::self += py::self)
        .def(py::self -= py::self)
        .def(py::self *= float())
        .def(py::self /= float())
        
        // Comparison operators
        .def(py::self == py::self)
        .def(py::self != py::self)
        
        // Methods
        .def("distance_to", &Point2D::distance_to, py::arg("other"))
        .def("distance_squared_to", &Point2D::distance_squared_to, py::arg("other"))
        .def("magnitude", &Point2D::magnitude)
        .def("magnitude_squared", &Point2D::magnitude_squared)
        .def("normalized", &Point2D::normalized)
        .def("dot", &Point2D::dot, py::arg("other"))
        .def("cross", &Point2D::cross, py::arg("other"))
        .def("angle_to", &Point2D::angle_to, py::arg("other"))
        .def("lerp", &Point2D::lerp, py::arg("other"), py::arg("t"))
        .def("nearly_equal", &Point2D::nearly_equal, py::arg("other"), py::arg("epsilon") = 1e-6f)
        
        // Static methods
        .def_static("zero", &Point2D::zero)
        .def_static("one", &Point2D::one)
        .def_static("unit_x", &Point2D::unit_x)
        .def_static("unit_y", &Point2D::unit_y)
        
        // String representation
        .def("__repr__", [](const Point2D& p) {
            return "Point2D(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
        })
        
        // Buffer protocol for numpy interop
        .def_buffer([](Point2D& p) -> py::buffer_info {
            return py::buffer_info(
                &p.x,                               // Pointer to buffer
                sizeof(float),                      // Size of one scalar
                py::format_descriptor<float>::format(), // Python struct-style format descriptor
                1,                                  // Number of dimensions
                {2},                               // Buffer dimensions
                {sizeof(float)}                    // Strides (in bytes) for each index
            );
        });
    
    // Color binding
    py::class_<Color>(m, "Color", py::buffer_protocol())
        .def(py::init<>())
        .def(py::init<uint32_t>(), py::arg("rgba"))
        .def(py::init<uint8_t, uint8_t, uint8_t, uint8_t>(), 
             py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 255)
        .def(py::init<float, float, float, float>(),
             py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 1.0f)
        .def(py::init<const Color&>())
        
        // Component access
        .def_property("r", [](const Color& c) { return c.r; },
                          [](Color& c, uint8_t v) { c.r = v; })
        .def_property("g", [](const Color& c) { return c.g; },
                          [](Color& c, uint8_t v) { c.g = v; })
        .def_property("b", [](const Color& c) { return c.b; },
                          [](Color& c, uint8_t v) { c.b = v; })
        .def_property("a", [](const Color& c) { return c.a; },
                          [](Color& c, uint8_t v) { c.a = v; })
        .def_property("rgba", [](const Color& c) { return c.rgba; },
                             [](Color& c, uint32_t v) { c.rgba = v; })
        
        // Float accessors - implement as lambdas since they're not in the header
        .def("r_float", [](const Color& c) { return c.r / 255.0f; })
        .def("g_float", [](const Color& c) { return c.g / 255.0f; })
        .def("b_float", [](const Color& c) { return c.b / 255.0f; })
        .def("a_float", [](const Color& c) { return c.a / 255.0f; })
        
        // Methods
        .def("blend_over", &Color::blend_over, py::arg("background"))
        .def("premultiply", &Color::premultiply)
        .def("to_hex", &Color::to_hex)
        .def("to_hsl", &Color::to_hsl)
        
        // Static methods
        .def_static("from_hex", &Color::from_hex, py::arg("hex"), py::arg("alpha") = 255)
        .def_static("from_hsl", &Color::from_hsl, py::arg("h"), py::arg("s"), py::arg("l"), py::arg("a") = 1.0f)
        
        // Comparison
        .def(py::self == py::self)
        .def(py::self != py::self)
        
        // String representation
        .def("__repr__", [](const Color& c) {
            return "Color(r=" + std::to_string(c.r) + 
                   ", g=" + std::to_string(c.g) + 
                   ", b=" + std::to_string(c.b) + 
                   ", a=" + std::to_string(c.a) + ")";
        });
    
    // Transform2D binding
    py::class_<Transform2D>(m, "Transform2D", py::buffer_protocol())
        .def(py::init<>())
        .def(py::init<float, float, float, float, float, float, float, float, float>(),
             py::arg("m00"), py::arg("m01"), py::arg("m02"),
             py::arg("m10"), py::arg("m11"), py::arg("m12"),
             py::arg("m20") = 0.0f, py::arg("m21") = 0.0f, py::arg("m22") = 1.0f)
        .def(py::init<const Transform2D&>())
        
        // Element access
        .def("__getitem__", [](const Transform2D& t, py::tuple idx) {
            if (idx.size() != 2) throw py::index_error("Transform2D requires 2 indices");
            int row = idx[0].cast<int>();
            int col = idx[1].cast<int>();
            if (row < 0 || row >= 3 || col < 0 || col >= 3)
                throw py::index_error("Transform2D index out of range");
            return t(row, col);
        })
        .def("__setitem__", [](Transform2D& t, py::tuple idx, float value) {
            if (idx.size() != 2) throw py::index_error("Transform2D requires 2 indices");
            int row = idx[0].cast<int>();
            int col = idx[1].cast<int>();
            if (row < 0 || row >= 3 || col < 0 || col >= 3)
                throw py::index_error("Transform2D index out of range");
            t(row, col) = value;
        })
        .def("at", py::overload_cast<int>(&Transform2D::at), py::arg("index"))
        
        // Factory methods
        .def_static("translate", &Transform2D::translate, py::arg("tx"), py::arg("ty"))
        .def_static("rotate", &Transform2D::rotate, py::arg("angle"))
        .def_static("scale", py::overload_cast<float, float>(&Transform2D::scale), 
                    py::arg("sx"), py::arg("sy"))
        .def_static("scale", py::overload_cast<float>(&Transform2D::scale), py::arg("s"))
        .def_static("shear", &Transform2D::shear, py::arg("shx"), py::arg("shy"))
        
        // Transform operations
        .def("translate_by", &Transform2D::translate_by, py::arg("tx"), py::arg("ty"))
        .def("rotate_by", &Transform2D::rotate_by, py::arg("angle"))
        .def("scale_by", &Transform2D::scale_by, py::arg("sx"), py::arg("sy"))
        
        // Matrix operations
        .def(py::self * py::self)
        .def(py::self *= py::self)
        .def("transform_point", &Transform2D::transform_point, py::arg("point"))
        .def("transform_vector", &Transform2D::transform_vector, py::arg("vector"))
        .def("__mul__", py::overload_cast<const Point2D&>(&Transform2D::operator*, py::const_), py::arg("point"))
        
        // Properties
        .def("determinant", &Transform2D::determinant)
        .def("is_invertible", &Transform2D::is_invertible)
        .def("inverse", &Transform2D::inverse)
        .def("decompose", [](const Transform2D& t) {
            float tx, ty, rotation, sx, sy;
            t.decompose(tx, ty, rotation, sx, sy);
            return py::make_tuple(tx, ty, rotation, sx, sy);
        })
        
        // Comparison
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("nearly_equal", &Transform2D::nearly_equal, py::arg("other"), py::arg("epsilon") = 1e-6f)
        
        // Buffer protocol for numpy interop
        .def_buffer([](Transform2D& t) -> py::buffer_info {
            return py::buffer_info(
                t.data(),                          // Pointer to buffer
                sizeof(float),                     // Size of one scalar
                py::format_descriptor<float>::format(), // Python struct-style format descriptor
                2,                                 // Number of dimensions
                {3, 3},                           // Buffer dimensions
                {3 * sizeof(float), sizeof(float)} // Strides
            );
        })
        
        // String representation
        .def("__repr__", [](const Transform2D& t) {
            std::stringstream ss;
            ss << "Transform2D([[" << t(0,0) << ", " << t(0,1) << ", " << t(0,2) << "],\n"
               << "            [" << t(1,0) << ", " << t(1,1) << ", " << t(1,2) << "],\n"
               << "            [" << t(2,0) << ", " << t(2,1) << ", " << t(2,2) << "]])";
            return ss.str();
        });
    
    // BoundingBox binding
    py::class_<BoundingBox>(m, "BoundingBox")
        .def(py::init<>())
        .def(py::init<float, float, float, float>(),
             py::arg("min_x"), py::arg("min_y"), py::arg("max_x"), py::arg("max_y"))
        .def(py::init<const Point2D&, const Point2D&>(), py::arg("p1"), py::arg("p2"))
        
        // Properties
        .def_readwrite("min_x", &BoundingBox::min_x)
        .def_readwrite("min_y", &BoundingBox::min_y)
        .def_readwrite("max_x", &BoundingBox::max_x)
        .def_readwrite("max_y", &BoundingBox::max_y)
        
        // Factory methods
        .def_static("from_center_half_extents", &BoundingBox::from_center_half_extents,
                    py::arg("center"), py::arg("half_width"), py::arg("half_height"))
        .def_static("from_center_size", &BoundingBox::from_center_size,
                    py::arg("center"), py::arg("width"), py::arg("height"))
        
        // Properties (methods)
        .def("is_empty", &BoundingBox::is_empty)
        .def("is_valid", &BoundingBox::is_valid)
        .def("width", &BoundingBox::width)
        .def("height", &BoundingBox::height)
        .def("area", &BoundingBox::area)
        .def("perimeter", &BoundingBox::perimeter)
        .def("center", &BoundingBox::center)
        
        // Corner access
        .def("min_corner", &BoundingBox::min_corner)
        .def("max_corner", &BoundingBox::max_corner)
        .def("top_left", &BoundingBox::top_left)
        .def("top_right", &BoundingBox::top_right)
        .def("bottom_left", &BoundingBox::bottom_left)
        .def("bottom_right", &BoundingBox::bottom_right)
        
        // Containment
        .def("contains", py::overload_cast<const Point2D&>(&BoundingBox::contains, py::const_), py::arg("point"))
        .def("contains", py::overload_cast<const BoundingBox&>(&BoundingBox::contains, py::const_), py::arg("other"))
        
        // Intersection
        .def("intersects", &BoundingBox::intersects, py::arg("other"))
        .def("intersection", &BoundingBox::intersection, py::arg("other"))
        
        // Union
        .def("union_with", &BoundingBox::union_with, py::arg("other"))
        
        // Expansion
        .def("expand", py::overload_cast<const Point2D&>(&BoundingBox::expand), py::arg("point"))
        .def("expand", py::overload_cast<const BoundingBox&>(&BoundingBox::expand), py::arg("other"))
        
        // Transformation
        .def("inflate", py::overload_cast<float>(&BoundingBox::inflate), py::arg("amount"))
        .def("inflate", py::overload_cast<float, float>(&BoundingBox::inflate), py::arg("dx"), py::arg("dy"))
        .def("scale", &BoundingBox::scale, py::arg("factor"))
        .def("translate", py::overload_cast<float, float>(&BoundingBox::translate), py::arg("dx"), py::arg("dy"))
        .def("translate", py::overload_cast<const Point2D&>(&BoundingBox::translate), py::arg("offset"))
        
        // Reset
        .def("reset", &BoundingBox::reset)
        
        // Comparison
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("nearly_equal", &BoundingBox::nearly_equal, py::arg("other"), py::arg("epsilon") = 1e-6f)
        
        // String representation
        .def("__repr__", [](const BoundingBox& bb) {
            return "BoundingBox(min_x=" + std::to_string(bb.min_x) + 
                   ", min_y=" + std::to_string(bb.min_y) +
                   ", max_x=" + std::to_string(bb.max_x) +
                   ", max_y=" + std::to_string(bb.max_y) + ")";
        });
    
    // Batch operations for Point2D
    m.def("batch_add_points", [](py::array_t<float> a, py::array_t<float> b, py::array_t<float> result) {
        auto a_info = a.request();
        auto b_info = b.request();
        auto r_info = result.request();
        
        if (a_info.ndim != 2 || a_info.shape[1] != 2 ||
            b_info.ndim != 2 || b_info.shape[1] != 2 ||
            r_info.ndim != 2 || r_info.shape[1] != 2) {
            throw std::runtime_error("Arrays must be shape (n, 2)");
        }
        
        size_t count = std::min({a_info.shape[0], b_info.shape[0], r_info.shape[0]});
        
        batch::add(
            reinterpret_cast<const Point2D*>(a_info.ptr),
            reinterpret_cast<const Point2D*>(b_info.ptr),
            reinterpret_cast<Point2D*>(r_info.ptr),
            count
        );
    }, py::arg("a"), py::arg("b"), py::arg("result"), 
    "Add two arrays of points element-wise using SIMD optimizations");
    
    // Batch operations for Color
    m.def("batch_blend_colors", [](py::array_t<uint32_t> foreground, py::array_t<uint32_t> background, py::array_t<uint32_t> result) {
        auto f_info = foreground.request();
        auto b_info = background.request();
        auto r_info = result.request();
        
        if (f_info.ndim != 1 || b_info.ndim != 1 || r_info.ndim != 1) {
            throw std::runtime_error("Arrays must be 1-dimensional");
        }
        
        size_t count = std::min({f_info.shape[0], b_info.shape[0], r_info.shape[0]});
        
        batch::blend_over(
            reinterpret_cast<const Color*>(f_info.ptr),
            reinterpret_cast<const Color*>(b_info.ptr),
            reinterpret_cast<Color*>(r_info.ptr),
            count
        );
    }, py::arg("foreground"), py::arg("background"), py::arg("result"),
    "Blend foreground colors over background colors using SIMD optimizations");
    
    // Batch operations for Transform2D
    m.def("batch_transform_points", [](const Transform2D& transform, py::array_t<float> points, py::array_t<float> result) {
        auto p_info = points.request();
        auto r_info = result.request();
        
        if (p_info.ndim != 2 || p_info.shape[1] != 2 ||
            r_info.ndim != 2 || r_info.shape[1] != 2) {
            throw std::runtime_error("Arrays must be shape (n, 2)");
        }
        
        size_t count = std::min(p_info.shape[0], r_info.shape[0]);
        
        batch::transform_points(
            transform,
            reinterpret_cast<const Point2D*>(p_info.ptr),
            reinterpret_cast<Point2D*>(r_info.ptr),
            count
        );
    }, py::arg("transform"), py::arg("points"), py::arg("result"),
    "Transform an array of points using SIMD optimizations");
    
    // Batch operations for BoundingBox
    m.def("batch_contains_points", [](const BoundingBox& bb, py::array_t<float> points) {
        auto p_info = points.request();
        
        if (p_info.ndim != 2 || p_info.shape[1] != 2) {
            throw std::runtime_error("Points array must be shape (n, 2)");
        }
        
        size_t count = p_info.shape[0];
        py::array_t<uint8_t> result(count);
        auto r_info = result.request();
        
        batch::contains_points(
            bb,
            reinterpret_cast<const Point2D*>(p_info.ptr),
            reinterpret_cast<uint8_t*>(r_info.ptr),
            count
        );
        
        return result;
    }, py::arg("bounding_box"), py::arg("points"),
    "Test if points are contained in bounding box using SIMD optimizations");
}