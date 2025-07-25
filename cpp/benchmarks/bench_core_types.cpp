#include <benchmark/benchmark.h>
#include <random>
#include <vector>

#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"
#include "claude_draw/memory/type_pools.h"
#include "claude_draw/batch/batch_operations.h"

using namespace claude_draw;
using namespace claude_draw::memory;
using namespace claude_draw::batch;

// Random number generator
static std::mt19937 rng(42);
static std::uniform_real_distribution<float> float_dist(-100.0f, 100.0f);
static std::uniform_int_distribution<uint8_t> byte_dist(0, 255);

// Point2D benchmarks
static void BM_Point2D_Construction(benchmark::State& state) {
    for (auto _ : state) {
        Point2D p(float_dist(rng), float_dist(rng));
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(BM_Point2D_Construction);

static void BM_Point2D_Addition(benchmark::State& state) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(3.0f, 4.0f);
    
    for (auto _ : state) {
        Point2D result = p1 + p2;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Point2D_Addition);

static void BM_Point2D_Distance(benchmark::State& state) {
    Point2D p1(float_dist(rng), float_dist(rng));
    Point2D p2(float_dist(rng), float_dist(rng));
    
    for (auto _ : state) {
        float dist = p1.distance_to(p2);
        benchmark::DoNotOptimize(dist);
    }
}
BENCHMARK(BM_Point2D_Distance);

static void BM_Point2D_Normalize(benchmark::State& state) {
    Point2D p(float_dist(rng), float_dist(rng));
    
    for (auto _ : state) {
        Point2D normalized = p.normalized();
        benchmark::DoNotOptimize(normalized);
    }
}
BENCHMARK(BM_Point2D_Normalize);

// Color benchmarks
static void BM_Color_Construction_RGBA(benchmark::State& state) {
    for (auto _ : state) {
        Color c(byte_dist(rng), byte_dist(rng), byte_dist(rng), byte_dist(rng));
        benchmark::DoNotOptimize(c);
    }
}
BENCHMARK(BM_Color_Construction_RGBA);

static void BM_Color_Construction_Packed(benchmark::State& state) {
    for (auto _ : state) {
        Color c(0xFF8040FF);
        benchmark::DoNotOptimize(c);
    }
}
BENCHMARK(BM_Color_Construction_Packed);

static void BM_Color_BlendOver(benchmark::State& state) {
    Color fg(255, 0, 0, 128);
    Color bg(0, 0, 255, 255);
    
    for (auto _ : state) {
        Color result = fg.blend_over(bg);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Color_BlendOver);

static void BM_Color_Premultiply(benchmark::State& state) {
    Color c(255, 128, 64, 200);
    
    for (auto _ : state) {
        Color premul = c.premultiply();
        benchmark::DoNotOptimize(premul);
    }
}
BENCHMARK(BM_Color_Premultiply);

// Transform2D benchmarks
static void BM_Transform2D_Construction(benchmark::State& state) {
    for (auto _ : state) {
        Transform2D t;
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_Transform2D_Construction);

static void BM_Transform2D_Multiplication(benchmark::State& state) {
    Transform2D t1 = Transform2D::rotate(0.5f);
    Transform2D t2 = Transform2D::scale(2.0f, 3.0f);
    
    for (auto _ : state) {
        Transform2D result = t1 * t2;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Transform2D_Multiplication);

static void BM_Transform2D_TransformPoint(benchmark::State& state) {
    Transform2D t = Transform2D::rotate(0.5f) * Transform2D::scale(2.0f, 3.0f);
    Point2D p(10.0f, 20.0f);
    
    for (auto _ : state) {
        Point2D result = t.transform_point(p);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Transform2D_TransformPoint);

static void BM_Transform2D_Inverse(benchmark::State& state) {
    Transform2D t = Transform2D::rotate(0.5f) * 
                    Transform2D::scale(2.0f, 3.0f) * 
                    Transform2D::translate(10.0f, 20.0f);
    
    for (auto _ : state) {
        Transform2D inv = t.inverse();
        benchmark::DoNotOptimize(inv);
    }
}
BENCHMARK(BM_Transform2D_Inverse);

// BoundingBox benchmarks
static void BM_BoundingBox_Construction(benchmark::State& state) {
    for (auto _ : state) {
        BoundingBox box(0.0f, 0.0f, 100.0f, 100.0f);
        benchmark::DoNotOptimize(box);
    }
}
BENCHMARK(BM_BoundingBox_Construction);

static void BM_BoundingBox_Contains(benchmark::State& state) {
    BoundingBox box(0.0f, 0.0f, 100.0f, 100.0f);
    Point2D p(float_dist(rng), float_dist(rng));
    
    for (auto _ : state) {
        bool contains = box.contains(p);
        benchmark::DoNotOptimize(contains);
    }
}
BENCHMARK(BM_BoundingBox_Contains);

static void BM_BoundingBox_Intersects(benchmark::State& state) {
    BoundingBox box1(0.0f, 0.0f, 100.0f, 100.0f);
    BoundingBox box2(50.0f, 50.0f, 150.0f, 150.0f);
    
    for (auto _ : state) {
        bool intersects = box1.intersects(box2);
        benchmark::DoNotOptimize(intersects);
    }
}
BENCHMARK(BM_BoundingBox_Intersects);

static void BM_BoundingBox_Union(benchmark::State& state) {
    BoundingBox box1(0.0f, 0.0f, 100.0f, 100.0f);
    BoundingBox box2(50.0f, 50.0f, 150.0f, 150.0f);
    
    for (auto _ : state) {
        BoundingBox result = box1.union_with(box2);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_BoundingBox_Union);

// Memory pool benchmarks
static void BM_ObjectPool_Allocate(benchmark::State& state) {
    ObjectPool<Point2D, 1024> pool;
    
    for (auto _ : state) {
        auto* p = pool.acquire(1.0f, 2.0f);
        benchmark::DoNotOptimize(p);
        pool.release(p);
    }
}
BENCHMARK(BM_ObjectPool_Allocate);

static void BM_StandardAllocate_Point2D(benchmark::State& state) {
    for (auto _ : state) {
        auto* p = new Point2D(1.0f, 2.0f);
        benchmark::DoNotOptimize(p);
        delete p;
    }
}
BENCHMARK(BM_StandardAllocate_Point2D);

static void BM_TypePools_MakePoint(benchmark::State& state) {
    for (auto _ : state) {
        auto p = TypePools::make_point(1.0f, 2.0f);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(BM_TypePools_MakePoint);

// Batch operation benchmarks
static void BM_BatchCreate_Points(benchmark::State& state) {
    const size_t count = state.range(0);
    
    for (auto _ : state) {
        auto points = BatchCreator::create_points(count, 0.0f, 0.0f);
        benchmark::DoNotOptimize(points);
        BatchCreator::destroy_batch(points);
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_BatchCreate_Points)->Range(100, 10000);

static void BM_ContiguousBatch_Points(benchmark::State& state) {
    const size_t count = state.range(0);
    
    for (auto _ : state) {
        ContiguousBatch<Point2D> batch(count);
        for (size_t i = 0; i < count; ++i) {
            batch.emplace(float(i), float(i));
        }
        benchmark::DoNotOptimize(batch.data());
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_ContiguousBatch_Points)->Range(100, 10000);

static void BM_BatchTransform_Points(benchmark::State& state) {
    const size_t count = state.range(0);
    ContiguousBatch<Point2D> points(count);
    
    for (size_t i = 0; i < count; ++i) {
        points.emplace(float_dist(rng), float_dist(rng));
    }
    
    Transform2D transform = Transform2D::rotate(0.5f) * Transform2D::scale(1.5f, 2.0f);
    
    for (auto _ : state) {
        BatchProcessor::transform_points(transform, points.as_span());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_BatchTransform_Points)->Range(100, 10000);

static void BM_BatchBlend_Colors(benchmark::State& state) {
    const size_t count = state.range(0);
    std::vector<Color> fg_colors(count);
    std::vector<Color> bg_colors(count);
    std::vector<Color> results(count);
    
    for (size_t i = 0; i < count; ++i) {
        fg_colors[i] = Color(255, 0, 0, 128);
        bg_colors[i] = Color(0, 0, 255, 255);
    }
    
    for (auto _ : state) {
        BatchProcessor::blend_colors(fg_colors.data(), bg_colors.data(), results.data(), count);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_BatchBlend_Colors)->Range(100, 10000);

// Comparison benchmarks: C++ vs simulated Python
static void BM_Python_Point_Distance(benchmark::State& state) {
    // Simulate Python-like performance with extra overhead
    float x1 = float_dist(rng), y1 = float_dist(rng);
    float x2 = float_dist(rng), y2 = float_dist(rng);
    
    for (auto _ : state) {
        // Simulate Python overhead with volatile operations
        volatile float dx = x2 - x1;
        volatile float dy = y2 - y1;
        volatile float dist = std::sqrt(dx * dx + dy * dy);
        benchmark::DoNotOptimize(dist);
    }
}
BENCHMARK(BM_Python_Point_Distance);

static void BM_CPP_Point_Distance(benchmark::State& state) {
    Point2D p1(float_dist(rng), float_dist(rng));
    Point2D p2(float_dist(rng), float_dist(rng));
    
    for (auto _ : state) {
        float dist = p1.distance_to(p2);
        benchmark::DoNotOptimize(dist);
    }
}
BENCHMARK(BM_CPP_Point_Distance);

// Complex operation benchmarks
static void BM_Complex_TransformChain(benchmark::State& state) {
    const size_t chain_length = state.range(0);
    std::vector<Transform2D> transforms;
    
    for (size_t i = 0; i < chain_length; ++i) {
        transforms.push_back(
            Transform2D::rotate(float_dist(rng) * 0.01f) *
            Transform2D::scale(1.0f + float_dist(rng) * 0.01f, 1.0f + float_dist(rng) * 0.01f) *
            Transform2D::translate(float_dist(rng) * 0.1f, float_dist(rng) * 0.1f)
        );
    }
    
    Point2D p(10.0f, 20.0f);
    
    for (auto _ : state) {
        Point2D result = p;
        for (const auto& t : transforms) {
            result = t.transform_point(result);
        }
        benchmark::DoNotOptimize(result);
    }
    
    state.SetComplexityN(chain_length);
}
BENCHMARK(BM_Complex_TransformChain)->Range(1, 32)->Complexity();

static void BM_Complex_ColorCompositing(benchmark::State& state) {
    const size_t layer_count = state.range(0);
    std::vector<Color> layers;
    
    for (size_t i = 0; i < layer_count; ++i) {
        layers.emplace_back(byte_dist(rng), byte_dist(rng), byte_dist(rng), 128);
    }
    
    Color background(255, 255, 255, 255);
    
    for (auto _ : state) {
        Color result = background;
        for (const auto& layer : layers) {
            result = layer.blend_over(result);
        }
        benchmark::DoNotOptimize(result);
    }
    
    state.SetComplexityN(layer_count);
}
BENCHMARK(BM_Complex_ColorCompositing)->Range(1, 32)->Complexity();