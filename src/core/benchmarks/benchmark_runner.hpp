/**
 * @file benchmark_runner.hpp
 * @brief Benchmark orchestration
 */

#ifndef BENCHMARK_RUNNER_HPP
#define BENCHMARK_RUNNER_HPP

#include "../allocators/base_allocator.hpp"
#include "../utils/timer.hpp"
#include "../utils/statistics.hpp"
#include <functional>
#include <vector>
#include <string>

namespace memory_engine {

struct BenchmarkConfig {
    size_t object_size = 256;
    size_t object_count = 10000;
    size_t iterations = 10;
    size_t alignment = 8;
    size_t thread_count = 1;
    bool randomize_order = false;
};

struct BenchmarkMetrics {
    BenchmarkResult allocation_time;
    BenchmarkResult deallocation_time;
    double throughput = 0;
    double peak_memory = 0;
    double fragmentation = 0;
    std::string allocator_name;
};

class BenchmarkRunner {
public:
    using ProgressCallback = std::function<void(int percent, const std::string& status)>;

    BenchmarkRunner() = default;

    void set_progress_callback(ProgressCallback callback) {
        m_progress_callback = callback;
    }

    BenchmarkMetrics run_allocation_benchmark(BaseAllocator& allocator, const BenchmarkConfig& config) {
        BenchmarkMetrics metrics;
        metrics.allocator_name = allocator.name();

        std::vector<double> alloc_times;
        std::vector<double> dealloc_times;
        std::vector<void*> pointers;
        pointers.reserve(config.object_count);

        for (size_t iter = 0; iter < config.iterations; ++iter) {
            allocator.reset();
            pointers.clear();

            // Allocation phase
            Timer alloc_timer;
            alloc_timer.start();
            
            for (size_t i = 0; i < config.object_count; ++i) {
                void* ptr = allocator.allocate(config.object_size, config.alignment);
                if (ptr) pointers.push_back(ptr);
            }
            
            alloc_timer.stop();
            alloc_times.push_back(alloc_timer.elapsed_ns() / config.object_count);

            metrics.peak_memory = std::max(metrics.peak_memory, 
                static_cast<double>(allocator.stats().peak_bytes_used));

            // Deallocation phase
            Timer dealloc_timer;
            dealloc_timer.start();
            
            for (void* ptr : pointers) {
                allocator.deallocate(ptr);
            }
            
            dealloc_timer.stop();
            dealloc_times.push_back(dealloc_timer.elapsed_ns() / pointers.size());

            if (m_progress_callback) {
                int percent = static_cast<int>((iter + 1) * 100 / config.iterations);
                m_progress_callback(percent, "Running iteration " + std::to_string(iter + 1));
            }
        }

        metrics.allocation_time = Statistics::analyze(alloc_times);
        metrics.deallocation_time = Statistics::analyze(dealloc_times);
        metrics.throughput = Statistics::throughput(config.object_count, metrics.allocation_time.mean);
        metrics.fragmentation = allocator.fragmentation_percentage();

        return metrics;
    }

private:
    ProgressCallback m_progress_callback;
};

} // namespace memory_engine

#endif // BENCHMARK_RUNNER_HPP
