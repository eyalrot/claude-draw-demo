#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "claude_draw/core/simd.h"

namespace py = pybind11;
using namespace claude_draw;

// Forward declarations for binding functions
void bind_core_types(py::module& m);
// void bind_shapes(py::module& m);
// void bind_containers(py::module& m);

PYBIND11_MODULE(_claude_draw_cpp, m) {
    m.doc() = "Claude Draw C++ optimization layer - high-performance 2D graphics primitives";
    
    // Version information
    m.attr("__version__") = "1.0.0";
    
    // CPU capabilities - runtime detection
    m.def("has_sse2", []() { return SimdCapabilities::has_sse2(); },
          "Check if SSE2 instructions are available");
    m.def("has_sse3", []() { return SimdCapabilities::has_sse3(); },
          "Check if SSE3 instructions are available");
    m.def("has_ssse3", []() { return SimdCapabilities::has_ssse3(); },
          "Check if SSSE3 instructions are available");
    m.def("has_sse41", []() { return SimdCapabilities::has_sse41(); },
          "Check if SSE4.1 instructions are available");
    m.def("has_sse42", []() { return SimdCapabilities::has_sse42(); },
          "Check if SSE4.2 instructions are available");
    m.def("has_avx", []() { return SimdCapabilities::has_avx(); },
          "Check if AVX instructions are available");
    m.def("has_avx2", []() { return SimdCapabilities::has_avx2(); },
          "Check if AVX2 instructions are available");
    m.def("has_avx512f", []() { return SimdCapabilities::has_avx512f(); },
          "Check if AVX512F instructions are available");
    m.def("has_fma", []() { return SimdCapabilities::has_fma(); },
          "Check if FMA instructions are available");
    
    m.def("get_simd_capabilities", []() { return SimdCapabilities::get_capabilities_string(); },
          "Get a string describing available SIMD capabilities");
    
    m.def("get_simd_level", []() -> std::string {
        switch (get_simd_level()) {
            case SimdLevel::AVX512: return "AVX512";
            case SimdLevel::AVX2: return "AVX2";
            case SimdLevel::AVX: return "AVX";
            case SimdLevel::SSE42: return "SSE4.2";
            case SimdLevel::SSE2: return "SSE2";
            case SimdLevel::SCALAR: return "SCALAR";
            default: return "UNKNOWN";
        }
    }, "Get the best available SIMD level");
    
    // Module organization
    auto core = m.def_submodule("core", "Core data types (Point2D, Color, Transform2D, BoundingBox)");
    auto shapes = m.def_submodule("shapes", "Shape primitives (Circle, Rectangle, Ellipse, Line)");
    auto containers = m.def_submodule("containers", "Container types (Group, Layer, Drawing)");
    auto memory = m.def_submodule("memory", "Memory management utilities");
    
    // Bind types
    bind_core_types(core);
    // bind_shapes(shapes);
    // bind_containers(containers);
    
    // Performance configuration
    m.def("set_thread_pool_size", [](size_t num_threads) {
        // TODO: Implement thread pool configuration
        py::print("Thread pool configuration not yet implemented");
        (void)num_threads;  // Suppress unused parameter warning
    }, py::arg("num_threads"), "Set the number of threads for parallel operations");
    
    m.def("get_memory_usage", []() -> size_t {
        // TODO: Implement memory tracking
        return 0;
    }, "Get current memory usage in bytes");
    
    // Optimization control
    m.def("enable_simd", [](bool enable) {
        // TODO: Implement SIMD control
        py::print("SIMD control not yet implemented");
        (void)enable;  // Suppress unused parameter warning
    }, py::arg("enable") = true, "Enable or disable SIMD optimizations");
}