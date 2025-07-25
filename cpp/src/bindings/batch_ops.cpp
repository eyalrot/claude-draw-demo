#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "claude_draw/batch/batch_operations.h"
#include "claude_draw/core/simd.h"

namespace py = pybind11;
using namespace claude_draw;
using namespace claude_draw::batch;

void init_batch_ops(py::module& m) {
    py::module batch_module = m.def_submodule("batch", "Batch operations for high performance");
    
    // Batch creation functions
    batch_module.def("create_points", 
        [](size_t count, float x, float y) {
            // Create points and convert to numpy array
            auto points = BatchCreator::create_points(count, x, y);
            
            // Create numpy array that owns the data
            py::array_t<float> result({count, size_t(2)});
            auto ptr = static_cast<float*>(result.mutable_unchecked<2>().mutable_data(0, 0));
            
            for (size_t i = 0; i < count; ++i) {
                ptr[i * 2] = points[i]->x;
                ptr[i * 2 + 1] = points[i]->y;
                delete points[i];
            }
            
            return result;
        },
        py::arg("count"), py::arg("x") = 0.0f, py::arg("y") = 0.0f,
        "Create a batch of points with the same coordinates");
        
    batch_module.def("create_points_from_arrays",
        [](py::array_t<float> x_coords, py::array_t<float> y_coords) {
            auto x = x_coords.unchecked<1>();
            auto y = y_coords.unchecked<1>();
            
            if (x.shape(0) != y.shape(0)) {
                throw std::runtime_error("X and Y arrays must have the same length");
            }
            
            size_t count = x.shape(0);
            auto points = BatchCreator::create_points(x.data(0), y.data(0), count);
            
            // Return as list of Point2D objects
            py::list result;
            for (auto* p : points) {
                result.append(py::cast(p, py::return_value_policy::take_ownership));
            }
            
            return result;
        },
        py::arg("x_coords"), py::arg("y_coords"),
        "Create points from coordinate arrays");
        
    batch_module.def("create_colors",
        [](size_t count, uint32_t rgba) {
            auto colors = BatchCreator::create_colors(count, rgba);
            
            // Create numpy array that owns the data
            py::array_t<uint32_t> result(count);
            auto ptr = static_cast<uint32_t*>(result.mutable_unchecked<1>().mutable_data(0));
            
            for (size_t i = 0; i < count; ++i) {
                ptr[i] = colors[i]->rgba;
                delete colors[i];
            }
            
            return result;
        },
        py::arg("count"), py::arg("rgba") = 0xFF000000,
        "Create a batch of colors with the same RGBA value");
        
    batch_module.def("create_colors_from_components",
        [](py::array_t<uint8_t> r, py::array_t<uint8_t> g, 
           py::array_t<uint8_t> b, py::array_t<uint8_t> a) {
            
            auto r_arr = r.unchecked<1>();
            auto g_arr = g.unchecked<1>();
            auto b_arr = b.unchecked<1>();
            
            size_t count = r_arr.shape(0);
            if (g_arr.shape(0) != count || b_arr.shape(0) != count) {
                throw std::runtime_error("R, G, B arrays must have the same length");
            }
            
            const uint8_t* a_ptr = nullptr;
            if (a.size() > 0) {
                auto a_arr = a.unchecked<1>();
                if (a_arr.shape(0) != count) {
                    throw std::runtime_error("Alpha array must have the same length as RGB");
                }
                a_ptr = a_arr.data(0);
            }
            
            auto colors = BatchCreator::create_colors(
                r_arr.data(0), g_arr.data(0), b_arr.data(0), a_ptr, count);
            
            // Return as list of Color objects
            py::list result;
            for (auto* c : colors) {
                result.append(py::cast(c, py::return_value_policy::take_ownership));
            }
            
            return result;
        },
        py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = py::array_t<uint8_t>(),
        "Create colors from component arrays");
    
    // Batch processing functions
    batch_module.def("transform_points_batch",
        [](const Transform2D& transform, py::array_t<float>& points) {
            if (points.ndim() != 2 || points.shape(1) != 2) {
                throw std::runtime_error("Points array must be shape (N, 2)");
            }
            
            auto pts = points.mutable_unchecked<2>();
            size_t count = pts.shape(0);
            
            // Apply transform to each point
            #pragma omp parallel for
            for (size_t i = 0; i < count; ++i) {
                float x = pts(i, 0);
                float y = pts(i, 1);
                Point2D p(x, y);
                Point2D transformed = transform.transform_point(p);
                pts(i, 0) = transformed.x;
                pts(i, 1) = transformed.y;
            }
        },
        py::arg("transform"), py::arg("points"),
        "Transform points in-place");
        
    batch_module.def("blend_colors_batch",
        [](py::array_t<uint32_t> fg_colors, py::array_t<uint32_t> bg_colors) {
            auto fg = fg_colors.unchecked<1>();
            auto bg = bg_colors.unchecked<1>();
            
            if (fg.shape(0) != bg.shape(0)) {
                throw std::runtime_error("Foreground and background arrays must have the same length");
            }
            
            size_t count = fg.shape(0);
            py::array_t<uint32_t> result(count);
            auto res = result.mutable_unchecked<1>();
            
            #pragma omp parallel for
            for (size_t i = 0; i < count; ++i) {
                Color fg_color(fg(i));
                Color bg_color(bg(i));
                Color blended = fg_color.blend_over(bg_color);
                res(i) = blended.rgba;
            }
            
            return result;
        },
        py::arg("fg_colors"), py::arg("bg_colors"),
        "Blend foreground colors over background colors");
        
    batch_module.def("contains_points_batch",
        [](const BoundingBox& box, py::array_t<float> points) {
            if (points.ndim() != 2 || points.shape(1) != 2) {
                throw std::runtime_error("Points array must be shape (N, 2)");
            }
            
            auto pts = points.unchecked<2>();
            size_t count = pts.shape(0);
            
            py::array_t<bool> result(count);
            auto res = result.mutable_unchecked<1>();
            
            #pragma omp parallel for
            for (size_t i = 0; i < count; ++i) {
                Point2D p(pts(i, 0), pts(i, 1));
                res(i) = box.contains(p);
            }
            
            return result;
        },
        py::arg("box"), py::arg("points"),
        "Check if points are contained in bounding box");
        
    batch_module.def("calculate_distances_batch",
        [](py::array_t<float> points1, py::array_t<float> points2) {
            if (points1.ndim() != 2 || points1.shape(1) != 2) {
                throw std::runtime_error("Points1 array must be shape (N, 2)");
            }
            if (points2.ndim() != 2 || points2.shape(1) != 2) {
                throw std::runtime_error("Points2 array must be shape (N, 2)");
            }
            
            auto p1 = points1.unchecked<2>();
            auto p2 = points2.unchecked<2>();
            
            if (p1.shape(0) != p2.shape(0)) {
                throw std::runtime_error("Point arrays must have the same length");
            }
            
            size_t count = p1.shape(0);
            py::array_t<float> result(count);
            auto res = result.mutable_unchecked<1>();
            
            // Calculate distances
            // TODO: Add SIMD optimization when batch_distances is implemented
            #pragma omp parallel for
            for (size_t i = 0; i < count; ++i) {
                Point2D pt1(p1(i, 0), p1(i, 1));
                Point2D pt2(p2(i, 0), p2(i, 1));
                res(i) = pt1.distance_to(pt2);
            }
            
            return result;
        },
        py::arg("points1"), py::arg("points2"),
        "Calculate distances between pairs of points");
    
    // ContiguousBatch for Point2D
    py::class_<ContiguousBatch<Point2D>>(batch_module, "PointBatch")
        .def(py::init<size_t>(), py::arg("capacity") = 1000)
        .def("add", [](ContiguousBatch<Point2D>& self, float x, float y) {
            return self.emplace(x, y);
        }, py::return_value_policy::reference_internal,
        py::arg("x"), py::arg("y"),
        "Add a point to the batch")
        .def("size", &ContiguousBatch<Point2D>::size)
        .def("clear", &ContiguousBatch<Point2D>::clear)
        .def("reserve", &ContiguousBatch<Point2D>::reserve)
        .def("as_array", [](ContiguousBatch<Point2D>& self) {
            // Return as numpy array view
            return py::array_t<float>(
                {self.size(), size_t(2)},
                {sizeof(Point2D), sizeof(float)},
                reinterpret_cast<float*>(self.data()),
                py::cast(self, py::return_value_policy::reference)
            );
        }, "Get points as numpy array (zero-copy view)");
        
    // ContiguousBatch for Color
    py::class_<ContiguousBatch<Color>>(batch_module, "ColorBatch")
        .def(py::init<size_t>(), py::arg("capacity") = 1000)
        .def("add", [](ContiguousBatch<Color>& self, uint32_t rgba) {
            return self.emplace(rgba);
        }, py::return_value_policy::reference_internal,
        py::arg("rgba"),
        "Add a color to the batch")
        .def("add_rgb", [](ContiguousBatch<Color>& self, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            return self.emplace(r, g, b, a);
        }, py::return_value_policy::reference_internal,
        py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 255,
        "Add a color from RGB components")
        .def("size", &ContiguousBatch<Color>::size)
        .def("clear", &ContiguousBatch<Color>::clear)
        .def("reserve", &ContiguousBatch<Color>::reserve)
        .def("as_array", [](ContiguousBatch<Color>& self) {
            // Return as numpy array view
            return py::array_t<uint32_t>(
                {self.size()},
                {sizeof(Color)},
                reinterpret_cast<uint32_t*>(self.data()),
                py::cast(self, py::return_value_policy::reference)
            );
        }, "Get colors as numpy array (zero-copy view)");
}