#include <benchmark/benchmark.h>
#include "bench_utils.h"
#include "claude_draw/shapes/circle_optimized.h"
#include "claude_draw/shapes/rectangle_optimized.h"
#include "claude_draw/shapes/ellipse_optimized.h"
#include "claude_draw/shapes/line_optimized.h"
#include "claude_draw/shapes/shape_batch_ops.h"
#include "claude_draw/shapes/shape_validation.h"
#include "claude_draw/shapes/unchecked_shapes.h"
#include <vector>
#include <random>
#include <memory>

using namespace claude_draw::shapes;
using namespace claude_draw;

// Random number generator for benchmarks
static std::mt19937 g_rng(42);
static std::uniform_real_distribution<float> g_coord_dist(-100.0f, 100.0f);
static std::uniform_real_distribution<float> g_size_dist(1.0f, 50.0f);

//=============================================================================
// Circle Benchmarks
//=============================================================================

static void BM_Circle_Create(benchmark::State& state) {
    for (auto _ : state) {
        Circle c(g_coord_dist(g_rng), g_coord_dist(g_rng), g_size_dist(g_rng));
        benchmark::DoNotOptimize(c);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Circle_Create);

static void BM_Circle_CreateUnchecked(benchmark::State& state) {
    for (auto _ : state) {
        auto c = unchecked::create_circle(g_coord_dist(g_rng), g_coord_dist(g_rng), g_size_dist(g_rng));
        benchmark::DoNotOptimize(c);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Circle_CreateUnchecked);

static void BM_Circle_Transform(benchmark::State& state) {
    Circle c(50.0f, 50.0f, 25.0f);
    Transform2D transform = Transform2D::translate(10.0f, 20.0f).scale(1.5f, 1.5f).rotate(M_PI/4);
    
    for (auto _ : state) {
        c.transform(transform);
        benchmark::DoNotOptimize(c);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Circle_Transform);

static void BM_Circle_ContainsPoint(benchmark::State& state) {
    Circle c(50.0f, 50.0f, 25.0f);
    Point2D p(60.0f, 60.0f);
    
    for (auto _ : state) {
        bool contains = c.contains_point(p);
        benchmark::DoNotOptimize(contains);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Circle_ContainsPoint);

static void BM_Circle_GetBounds(benchmark::State& state) {
    Circle c(50.0f, 50.0f, 25.0f);
    
    for (auto _ : state) {
        auto bounds = c.get_bounds();
        benchmark::DoNotOptimize(bounds);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Circle_GetBounds);

//=============================================================================
// Rectangle Benchmarks
//=============================================================================

static void BM_Rectangle_Create(benchmark::State& state) {
    for (auto _ : state) {
        Rectangle r(g_coord_dist(g_rng), g_coord_dist(g_rng), 
                   g_size_dist(g_rng), g_size_dist(g_rng));
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Rectangle_Create);

static void BM_Rectangle_CreateUnchecked(benchmark::State& state) {
    for (auto _ : state) {
        auto r = unchecked::create_rectangle(g_coord_dist(g_rng), g_coord_dist(g_rng), 
                                            g_size_dist(g_rng), g_size_dist(g_rng));
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Rectangle_CreateUnchecked);

static void BM_Rectangle_Intersection(benchmark::State& state) {
    Rectangle r1(10.0f, 10.0f, 30.0f, 30.0f);
    Rectangle r2(20.0f, 20.0f, 30.0f, 30.0f);
    
    for (auto _ : state) {
        auto intersection = r1.intersection(r2);
        benchmark::DoNotOptimize(intersection);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Rectangle_Intersection);

static void BM_Rectangle_Union(benchmark::State& state) {
    Rectangle r1(10.0f, 10.0f, 30.0f, 30.0f);
    Rectangle r2(20.0f, 20.0f, 30.0f, 30.0f);
    
    for (auto _ : state) {
        auto union_rect = r1.union_with(r2);
        benchmark::DoNotOptimize(union_rect);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Rectangle_Union);

//=============================================================================
// Ellipse Benchmarks
//=============================================================================

static void BM_Ellipse_Create(benchmark::State& state) {
    for (auto _ : state) {
        Ellipse e(g_coord_dist(g_rng), g_coord_dist(g_rng), 
                 g_size_dist(g_rng), g_size_dist(g_rng));
        benchmark::DoNotOptimize(e);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ellipse_Create);

static void BM_Ellipse_Perimeter(benchmark::State& state) {
    Ellipse e(50.0f, 50.0f, 30.0f, 20.0f);
    
    for (auto _ : state) {
        float perimeter = e.perimeter();
        benchmark::DoNotOptimize(perimeter);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ellipse_Perimeter);

//=============================================================================
// Line Benchmarks
//=============================================================================

static void BM_Line_Create(benchmark::State& state) {
    for (auto _ : state) {
        Line l(g_coord_dist(g_rng), g_coord_dist(g_rng), 
               g_coord_dist(g_rng), g_coord_dist(g_rng));
        benchmark::DoNotOptimize(l);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Line_Create);

static void BM_Line_Intersection(benchmark::State& state) {
    Line l1(0.0f, 0.0f, 100.0f, 100.0f);
    Line l2(0.0f, 100.0f, 100.0f, 0.0f);
    
    for (auto _ : state) {
        Point2D intersection;
        bool intersects = l1.intersects_line(l2, &intersection);
        benchmark::DoNotOptimize(intersects);
        benchmark::DoNotOptimize(intersection);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Line_Intersection);

static void BM_Line_DistanceToPoint(benchmark::State& state) {
    Line l(0.0f, 0.0f, 100.0f, 0.0f);
    Point2D p(50.0f, 10.0f);
    
    for (auto _ : state) {
        float distance = l.distance_to_point(p);
        benchmark::DoNotOptimize(distance);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Line_DistanceToPoint);

//=============================================================================
// Batch Operation Benchmarks
//=============================================================================

static void BM_Batch_CreateCircles(benchmark::State& state) {
    const size_t count = state.range(0);
    
    for (auto _ : state) {
        std::vector<Circle> circles;
        circles.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            circles.emplace_back(i * 10.0f, i * 10.0f, 5.0f);
        }
        
        benchmark::DoNotOptimize(circles.data());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_Batch_CreateCircles)->Range(100, 10000);

static void BM_Batch_TransformCircles(benchmark::State& state) {
    const size_t count = state.range(0);
    std::vector<Circle> circles;
    circles.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(i * 10.0f, i * 10.0f, 5.0f);
    }
    
    Transform2D transform = Transform2D::scale(2.0f, 2.0f);
    
    for (auto _ : state) {
        Circle::batch_transform(circles.data(), count, transform);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_Batch_TransformCircles)->Range(100, 10000);

static void BM_Batch_CalculateBounds(benchmark::State& state) {
    const size_t count = state.range(0);
    std::vector<CircleShape> circles;
    circles.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(i * 10.0f, i * 10.0f, 5.0f);
    }
    
    std::vector<BoundingBox> results(count);
    
    for (auto _ : state) {
        shapes::batch::simd::calculate_circle_bounds_simd(circles.data(), count, results.data());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_Batch_CalculateBounds)->Range(100, 10000);

//=============================================================================
// Mixed Shape Operations
//=============================================================================

static void BM_MixedShapes_CalculateBounds(benchmark::State& state) {
    const size_t shapes_per_type = state.range(0) / 4;
    const size_t total_shapes = shapes_per_type * 4;
    
    // Create mixed shape collection
    std::vector<std::unique_ptr<void, void(*)(void*)>> shape_storage;
    std::vector<const void*> shape_ptrs;
    std::vector<ShapeType> shape_types;
    
    // Add circles
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto circle = std::make_unique<Circle>(i * 10.0f, i * 10.0f, 5.0f);
        shape_ptrs.push_back(circle.get());
        shape_types.push_back(ShapeType::Circle);
        shape_storage.emplace_back(circle.release(), [](void* p) { delete static_cast<Circle*>(p); });
    }
    
    // Add rectangles
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto rect = std::make_unique<Rectangle>(i * 10.0f, i * 10.0f, 20.0f, 20.0f);
        shape_ptrs.push_back(rect.get());
        shape_types.push_back(ShapeType::Rectangle);
        shape_storage.emplace_back(rect.release(), [](void* p) { delete static_cast<Rectangle*>(p); });
    }
    
    // Add ellipses
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto ellipse = std::make_unique<Ellipse>(i * 10.0f, i * 10.0f, 15.0f, 10.0f);
        shape_ptrs.push_back(ellipse.get());
        shape_types.push_back(ShapeType::Ellipse);
        shape_storage.emplace_back(ellipse.release(), [](void* p) { delete static_cast<Ellipse*>(p); });
    }
    
    // Add lines
    for (size_t i = 0; i < shapes_per_type; ++i) {
        auto line = std::make_unique<Line>(i * 10.0f, i * 10.0f, (i+1) * 10.0f, (i+1) * 10.0f);
        shape_ptrs.push_back(line.get());
        shape_types.push_back(ShapeType::Line);
        shape_storage.emplace_back(line.release(), [](void* p) { delete static_cast<Line*>(p); });
    }
    
    std::vector<BoundingBox> results(total_shapes);
    
    for (auto _ : state) {
        shapes::batch::calculate_bounds(shape_ptrs.data(), shape_types.data(), total_shapes, results.data());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * total_shapes);
}
BENCHMARK(BM_MixedShapes_CalculateBounds)->Range(100, 10000);

//=============================================================================
// Validation Mode Benchmarks
//=============================================================================

static void BM_Circle_Create_WithValidation(benchmark::State& state) {
    ValidationScope scope(ValidationMode::Full);
    
    for (auto _ : state) {
        try {
            Circle c(g_coord_dist(g_rng), g_coord_dist(g_rng), g_size_dist(g_rng));
            benchmark::DoNotOptimize(c);
        } catch (...) {
            // Handle validation errors
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Circle_Create_WithValidation);

static void BM_Circle_Create_NoValidation(benchmark::State& state) {
    ValidationScope scope(ValidationMode::None);
    
    for (auto _ : state) {
        Circle c(g_coord_dist(g_rng), g_coord_dist(g_rng), g_size_dist(g_rng));
        benchmark::DoNotOptimize(c);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Circle_Create_NoValidation);

//=============================================================================
// Memory Layout Benchmarks
//=============================================================================

static void BM_ShapeSize_Verification(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Verify all shapes are exactly 32 bytes
        static_assert(sizeof(CircleShape) == 32);
        static_assert(sizeof(RectangleShape) == 32);
        static_assert(sizeof(EllipseShape) == 32);
        static_assert(sizeof(LineShape) == 32);
        
        // Verify wrapper classes have minimal overhead
        static_assert(sizeof(Circle) == sizeof(CircleShape));
        static_assert(sizeof(Rectangle) == sizeof(RectangleShape));
        static_assert(sizeof(Ellipse) == sizeof(EllipseShape));
        static_assert(sizeof(Line) == sizeof(LineShape));
        
        state.ResumeTiming();
        
        // Dummy operation
        benchmark::DoNotOptimize(sizeof(Circle));
    }
}
BENCHMARK(BM_ShapeSize_Verification);

//=============================================================================
// ShapeCollection Benchmarks
//=============================================================================

static void BM_ShapeCollection_AddShapes(benchmark::State& state) {
    const size_t count = state.range(0);
    
    for (auto _ : state) {
        shapes::batch::ShapeCollection collection;
        
        for (size_t i = 0; i < count; ++i) {
            collection.add_circle(std::make_unique<Circle>(i * 10.0f, i * 10.0f, 5.0f));
        }
        
        benchmark::DoNotOptimize(collection.size());
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_ShapeCollection_AddShapes)->Range(100, 10000);

static void BM_ShapeCollection_TransformAll(benchmark::State& state) {
    const size_t count = state.range(0);
    shapes::batch::ShapeCollection collection;
    
    // Fill collection
    for (size_t i = 0; i < count; ++i) {
        collection.add_circle(std::make_unique<Circle>(i * 10.0f, i * 10.0f, 5.0f));
        collection.add_rectangle(std::make_unique<Rectangle>(i * 10.0f, i * 10.0f, 20.0f, 20.0f));
    }
    
    Transform2D transform = Transform2D::rotate(M_PI / 4);
    
    for (auto _ : state) {
        collection.transform_all(transform);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * collection.size());
}
BENCHMARK(BM_ShapeCollection_TransformAll)->Range(100, 1000);

//=============================================================================
// Cache Performance Benchmarks
//=============================================================================

static void BM_CircleArray_Sequential(benchmark::State& state) {
    const size_t count = state.range(0);
    std::vector<Circle> circles;
    circles.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(i * 10.0f, i * 10.0f, 5.0f);
    }
    
    for (auto _ : state) {
        float sum = 0.0f;
        for (const auto& circle : circles) {
            sum += circle.get_radius();
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetItemsProcessed(state.iterations() * count);
    state.SetBytesProcessed(state.iterations() * count * sizeof(Circle));
}
BENCHMARK(BM_CircleArray_Sequential)->Range(1000, 100000);

static void BM_CircleArray_Random(benchmark::State& state) {
    const size_t count = state.range(0);
    std::vector<Circle> circles;
    std::vector<size_t> indices;
    circles.reserve(count);
    indices.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        circles.emplace_back(i * 10.0f, i * 10.0f, 5.0f);
        indices.push_back(i);
    }
    
    // Shuffle indices for random access
    std::shuffle(indices.begin(), indices.end(), g_rng);
    
    for (auto _ : state) {
        float sum = 0.0f;
        for (size_t idx : indices) {
            sum += circles[idx].get_radius();
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetItemsProcessed(state.iterations() * count);
    state.SetBytesProcessed(state.iterations() * count * sizeof(Circle));
}
BENCHMARK(BM_CircleArray_Random)->Range(1000, 100000);

// Main function is provided by benchmark library