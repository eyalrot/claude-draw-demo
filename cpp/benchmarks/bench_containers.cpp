#include <benchmark/benchmark.h>
#include "claude_draw/containers/soa_container.h"
#include "claude_draw/containers/spatial_index.h"
#include "claude_draw/containers/parallel_visitor.h"
#include "claude_draw/containers/incremental_bounds.h"
#include "claude_draw/containers/cow_container.h"
#include "claude_draw/containers/bulk_operations.h"
#include <random>
#include <vector>

using namespace claude_draw::containers;
using namespace claude_draw::shapes;
using namespace claude_draw;

// Helper to generate random shapes
class ShapeGenerator {
public:
    ShapeGenerator() : rng_(42), pos_dist_(-1000.0f, 1000.0f), size_dist_(1.0f, 100.0f) {}
    
    Circle random_circle() {
        return Circle(pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_));
    }
    
    Rectangle random_rectangle() {
        float x1 = pos_dist_(rng_);
        float y1 = pos_dist_(rng_);
        return Rectangle(x1, y1, x1 + size_dist_(rng_), y1 + size_dist_(rng_));
    }
    
    Ellipse random_ellipse() {
        return Ellipse(pos_dist_(rng_), pos_dist_(rng_), size_dist_(rng_), size_dist_(rng_) * 0.5f);
    }
    
    Line random_line() {
        return Line(pos_dist_(rng_), pos_dist_(rng_), pos_dist_(rng_), pos_dist_(rng_));
    }
    
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<float> pos_dist_;
    std::uniform_real_distribution<float> size_dist_;
};

// Benchmark: SoA Container insertion
static void BM_SoAContainer_AddCircle(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        SoAContainer container;
        for (int i = 0; i < state.range(0); ++i) {
            container.add_circle(gen.random_circle());
        }
        benchmark::DoNotOptimize(container.size());
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SoAContainer_AddCircle)->Range(100, 10000);

// Benchmark: SoA Container mixed insertion
static void BM_SoAContainer_AddMixed(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        SoAContainer container;
        for (int i = 0; i < state.range(0); ++i) {
            switch (i % 4) {
                case 0: container.add_circle(gen.random_circle()); break;
                case 1: container.add_rectangle(gen.random_rectangle()); break;
                case 2: container.add_ellipse(gen.random_ellipse()); break;
                case 3: container.add_line(gen.random_line()); break;
            }
        }
        benchmark::DoNotOptimize(container.size());
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SoAContainer_AddMixed)->Range(100, 10000);

// Benchmark: SoA Container removal
static void BM_SoAContainer_Remove(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        state.PauseTiming();
        SoAContainer container;
        std::vector<uint32_t> ids;
        for (int i = 0; i < state.range(0); ++i) {
            ids.push_back(container.add_circle(gen.random_circle()));
        }
        state.ResumeTiming();
        
        // Remove half the shapes
        for (size_t i = 0; i < ids.size() / 2; ++i) {
            container.remove(ids[i]);
        }
        benchmark::DoNotOptimize(container.size());
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0) / 2);
}
BENCHMARK(BM_SoAContainer_Remove)->Range(100, 10000);

// Benchmark: Bounds calculation
static void BM_SoAContainer_GetBounds(benchmark::State& state) {
    ShapeGenerator gen;
    SoAContainer container;
    
    // Pre-populate container
    for (int i = 0; i < state.range(0); ++i) {
        container.add_circle(gen.random_circle());
    }
    
    for (auto _ : state) {
        BoundingBox bounds = container.get_bounds();
        benchmark::DoNotOptimize(bounds);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SoAContainer_GetBounds)->Range(100, 100000);

// Benchmark: Spatial index insertion
static void BM_RTree_Insert(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        RTree<uint32_t> index;
        for (int i = 0; i < state.range(0); ++i) {
            Circle c = gen.random_circle();
            float x = c.get_center_x();
            float y = c.get_center_y();
            float size = 50.0f;
            BoundingBox bounds(x, y, x + size, y + size);
            index.insert(bounds, static_cast<uint32_t>(i));
        }
        benchmark::DoNotOptimize(index.size());
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_RTree_Insert)->Range(100, 10000);

// Benchmark: Spatial index query
static void BM_RTree_Query(benchmark::State& state) {
    ShapeGenerator gen;
    RTree<uint32_t> index;
    
    // Pre-populate index
    for (int i = 0; i < state.range(0); ++i) {
        Circle c = gen.random_circle();
        float x = c.get_center_x();
        float y = c.get_center_y();
        float size = 50.0f;
        BoundingBox bounds(x, y, x + size, y + size);
        index.insert(bounds, static_cast<uint32_t>(i));
    }
    
    for (auto _ : state) {
        BoundingBox query(-500, -500, 500, 500);
        size_t count = 0;
        index.query(query, [&count](const uint32_t&) {
            count++;
            return true;
        });
        benchmark::DoNotOptimize(count);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RTree_Query)->Range(100, 10000);

// Benchmark: STR bulk loading
// NOTE: STR bulk loading not yet implemented in RTree
// static void BM_RTree_STRLoad(benchmark::State& state) {
//     ShapeGenerator gen;
//     
//     for (auto _ : state) {
//         state.PauseTiming();
//         std::vector<std::pair<BoundingBox, uint32_t>> items;
//         for (int i = 0; i < state.range(0); ++i) {
//             Circle c = gen.random_circle();
//             float x = c.get_center_x();
//             float y = c.get_center_y();
//             float size = 50.0f;
//             BoundingBox bounds(x, y, x + size, y + size);
//             items.emplace_back(bounds, static_cast<uint32_t>(i));
//         }
//         state.ResumeTiming();
//         
//         RTree<uint32_t> index;
//         index.str_load(items);
//         benchmark::DoNotOptimize(index.size());
//     }
//     
//     state.SetItemsProcessed(state.iterations() * state.range(0));
// }
// BENCHMARK(BM_RTree_STRLoad)->Range(1000, 100000);

// Benchmark: Parallel visitor
static void BM_ParallelVisitor(benchmark::State& state) {
    ShapeGenerator gen;
    SoAContainer container;
    
    // Pre-populate container
    for (int i = 0; i < state.range(0); ++i) {
        container.add_circle(gen.random_circle());
    }
    
    ParallelVisitor visitor;
    
    for (auto _ : state) {
        std::atomic<double> total_area(0.0);
        visitor.visit_circles(container, [&total_area](CircleShape& circle, size_t) {
            double area = M_PI * circle.radius * circle.radius;
            total_area.fetch_add(area, std::memory_order_relaxed);
        });
        benchmark::DoNotOptimize(total_area.load());
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ParallelVisitor)->Range(1000, 100000);

// Benchmark: Incremental bounds tracking
static void BM_IncrementalBounds(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        IncrementalBounds bounds_tracker;
        
        for (int i = 0; i < state.range(0); ++i) {
            auto circle = gen.random_circle();
            bounds_tracker.add_shape(circle.get_bounds());
        }
        
        BoundingBox bounds = bounds_tracker.get_bounds();
        benchmark::DoNotOptimize(bounds);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_IncrementalBounds)->Range(100, 10000);

// Benchmark: CoW container copy
static void BM_CoWContainer_Copy(benchmark::State& state) {
    ShapeGenerator gen;
    CoWContainer container;
    
    // Pre-populate container
    for (int i = 0; i < state.range(0); ++i) {
        container.add_circle(gen.random_circle());
    }
    
    for (auto _ : state) {
        CoWContainer copy = container;
        benchmark::DoNotOptimize(copy.size());
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CoWContainer_Copy)->Range(100, 10000);

// Benchmark: CoW container write trigger
static void BM_CoWContainer_WriteTrigger(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        state.PauseTiming();
        CoWContainer original;
        for (int i = 0; i < state.range(0); ++i) {
            original.add_circle(gen.random_circle());
        }
        CoWContainer copy = original;
        state.ResumeTiming();
        
        // Trigger copy-on-write
        copy.add_circle(gen.random_circle());
        benchmark::DoNotOptimize(copy.size());
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CoWContainer_WriteTrigger)->Range(100, 10000);

// Benchmark: Bulk insertion
static void BM_BulkOperations_Insert(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<Circle> circles;
        circles.reserve(state.range(0));
        for (int i = 0; i < state.range(0); ++i) {
            circles.push_back(gen.random_circle());
        }
        SoAContainer container;
        state.ResumeTiming();
        
        auto ids = BulkOperations::bulk_insert(container, std::span<const Circle>(circles));
        benchmark::DoNotOptimize(ids.size());
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_BulkOperations_Insert)->Range(100, 10000);

// Benchmark: Bulk transform
static void BM_BulkOperations_Transform(benchmark::State& state) {
    ShapeGenerator gen;
    SoAContainer container;
    
    // Pre-populate container
    for (int i = 0; i < state.range(0); ++i) {
        container.add_circle(gen.random_circle());
    }
    
    for (auto _ : state) {
        size_t count = BulkOperations::bulk_transform<CircleShape>(
            container,
            [](CircleShape& circle) {
                circle.radius *= 1.01f;  // Scale by 1%
            });
        benchmark::DoNotOptimize(count);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_BulkOperations_Transform)->Range(100, 10000);

// Benchmark: Full container workflow
static void BM_ContainerWorkflow(benchmark::State& state) {
    ShapeGenerator gen;
    
    for (auto _ : state) {
        SoAContainer container;
        RTree<uint32_t> spatial_index;
        
        // Insert shapes
        std::vector<uint32_t> ids;
        for (int i = 0; i < state.range(0); ++i) {
            auto circle = gen.random_circle();
            uint32_t id = container.add_circle(circle);
            ids.push_back(id);
            
            BoundingBox bounds = circle.get_bounds();
            spatial_index.insert(bounds, id);
        }
        
        // Query spatial index
        BoundingBox query(-200, -200, 200, 200);
        std::vector<uint32_t> results;
        spatial_index.query(query, [&results](const uint32_t& id) {
            results.push_back(id);
            return true;
        });
        
        // Calculate bounds
        BoundingBox bounds = container.get_bounds();
        
        benchmark::DoNotOptimize(results.size());
        benchmark::DoNotOptimize(bounds);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ContainerWorkflow)->Range(100, 10000);