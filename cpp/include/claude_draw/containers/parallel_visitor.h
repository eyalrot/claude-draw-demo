#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <future>
#include <algorithm>
#include "../shapes/shape_base_optimized.h"
#include "soa_container.h"

namespace claude_draw {
namespace containers {

/**
 * @brief Simplified parallel visitor for multi-threaded shape traversal
 * 
 * Provides basic parallel iteration over shapes in a SoA container.
 */
class ParallelVisitor {
public:
    // Configuration for parallel execution
    struct Config {
        size_t min_chunk_size;
        size_t thread_count;  // 0 = auto-detect
        
        Config() : min_chunk_size(64), thread_count(0) {}
    };
    
    ParallelVisitor(const Config& config = Config())
        : config_(config) {
        if (config_.thread_count == 0) {
            config_.thread_count = std::thread::hardware_concurrency();
            if (config_.thread_count == 0) {
                config_.thread_count = 1;
            }
        }
    }
    
    /**
     * @brief Visit all circles in parallel
     */
    template<typename Func>
    void visit_circles(SoAContainer& container, Func func) {
        const auto& circles = container.get_circles();
        parallel_for(circles, func);
    }
    
    /**
     * @brief Visit all rectangles in parallel
     */
    template<typename Func>
    void visit_rectangles(SoAContainer& container, Func func) {
        const auto& rectangles = container.get_rectangles();
        parallel_for(rectangles, func);
    }
    
    /**
     * @brief Visit all ellipses in parallel
     */
    template<typename Func>
    void visit_ellipses(SoAContainer& container, Func func) {
        const auto& ellipses = container.get_ellipses();
        parallel_for(ellipses, func);
    }
    
    /**
     * @brief Visit all lines in parallel
     */
    template<typename Func>
    void visit_lines(SoAContainer& container, Func func) {
        const auto& lines = container.get_lines();
        parallel_for(lines, func);
    }
    
    /**
     * @brief Parallel reduction over circles
     */
    template<typename T, typename ReduceFunc, typename CombineFunc>
    T reduce_circles(const SoAContainer& container, T identity,
                     ReduceFunc reduce_func, CombineFunc combine_func) {
        const auto& circles = container.get_circles();
        return parallel_reduce(circles, identity, reduce_func, combine_func);
    }
    
    /**
     * @brief Parallel bounds calculation
     */
    BoundingBox calculate_bounds_parallel(const SoAContainer& container) {
        std::vector<std::future<BoundingBox>> futures;
        
        // Circles
        futures.push_back(std::async(std::launch::async, [&]() {
            const auto& circles = container.get_circles();
            BoundingBox bounds;
            for (const auto& c : circles) {
                bounds = bounds.union_with(shapes::Circle(c).get_bounds());
            }
            return bounds;
        }));
        
        // Rectangles
        futures.push_back(std::async(std::launch::async, [&]() {
            const auto& rectangles = container.get_rectangles();
            BoundingBox bounds;
            for (const auto& r : rectangles) {
                bounds = bounds.union_with(shapes::Rectangle(r).get_bounds());
            }
            return bounds;
        }));
        
        // Ellipses
        futures.push_back(std::async(std::launch::async, [&]() {
            const auto& ellipses = container.get_ellipses();
            BoundingBox bounds;
            for (const auto& e : ellipses) {
                bounds = bounds.union_with(shapes::Ellipse(e).get_bounds());
            }
            return bounds;
        }));
        
        // Lines
        futures.push_back(std::async(std::launch::async, [&]() {
            const auto& lines = container.get_lines();
            BoundingBox bounds;
            for (const auto& l : lines) {
                bounds = bounds.union_with(shapes::Line(l).get_bounds());
            }
            return bounds;
        }));
        
        // Combine results
        BoundingBox final_bounds;
        for (auto& future : futures) {
            final_bounds = final_bounds.union_with(future.get());
        }
        
        return final_bounds;
    }
    
    /**
     * @brief Visit sorted shapes (sequential for now)
     */
    template<typename Func>
    void visit_sorted(SoAContainer& container, Func func) {
        container.for_each_sorted(func);
    }
    
private:
    Config config_;
    
    // Generic parallel for implementation
    template<typename Container, typename Func>
    void parallel_for(const Container& items, Func func) {
        size_t total_count = items.size();
        if (total_count < config_.min_chunk_size * 2) {
            // Sequential execution for small datasets
            for (size_t i = 0; i < total_count; ++i) {
                func(const_cast<typename Container::value_type&>(items[i]), i);
            }
            return;
        }
        
        size_t thread_count = std::min(config_.thread_count,
                                      (total_count + config_.min_chunk_size - 1) / config_.min_chunk_size);
        
        std::vector<std::thread> threads;
        threads.reserve(thread_count);
        
        for (size_t t = 0; t < thread_count; ++t) {
            size_t start = t * total_count / thread_count;
            size_t end = (t + 1) * total_count / thread_count;
            
            threads.emplace_back([&, start, end]() {
                for (size_t i = start; i < end; ++i) {
                    func(const_cast<typename Container::value_type&>(items[i]), i);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    // Generic parallel reduce implementation
    template<typename Container, typename T, typename ReduceFunc, typename CombineFunc>
    T parallel_reduce(const Container& items, T identity,
                     ReduceFunc reduce_func, CombineFunc combine_func) {
        size_t total_count = items.size();
        if (total_count == 0) return identity;
        
        if (total_count < config_.min_chunk_size * 2) {
            // Sequential reduction
            T result = identity;
            for (const auto& item : items) {
                result = combine_func(result, reduce_func(item));
            }
            return result;
        }
        
        size_t thread_count = std::min(config_.thread_count,
                                      (total_count + config_.min_chunk_size - 1) / config_.min_chunk_size);
        
        std::vector<std::future<T>> futures;
        futures.reserve(thread_count);
        
        for (size_t t = 0; t < thread_count; ++t) {
            size_t start = t * total_count / thread_count;
            size_t end = (t + 1) * total_count / thread_count;
            
            futures.push_back(std::async(std::launch::async, [&, start, end]() {
                T local_result = identity;
                for (size_t i = start; i < end; ++i) {
                    local_result = combine_func(local_result, reduce_func(items[i]));
                }
                return local_result;
            }));
        }
        
        // Combine results
        T final_result = identity;
        for (auto& future : futures) {
            final_result = combine_func(final_result, future.get());
        }
        
        return final_result;
    }
};

} // namespace containers
} // namespace claude_draw