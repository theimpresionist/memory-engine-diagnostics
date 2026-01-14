/**
 * @file main.cpp
 * @brief Native test entry point
 */

#include "core/engine.hpp"
#include <iostream>
#include <iomanip>

using namespace memory_engine;

void print_separator() {
    std::cout << std::string(60, '=') << std::endl;
}

void print_benchmark_results(const BenchmarkMetrics& metrics) {
    std::cout << "\nAllocator: " << metrics.allocator_name << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Mean Alloc Time:   " << metrics.allocation_time.mean << " ns" << std::endl;
    std::cout << "  Median Alloc Time: " << metrics.allocation_time.median << " ns" << std::endl;
    std::cout << "  Min/Max:           " << metrics.allocation_time.min << " / " 
              << metrics.allocation_time.max << " ns" << std::endl;
    std::cout << "  Std Dev:           " << metrics.allocation_time.std_dev << " ns" << std::endl;
    std::cout << "  Throughput:        " << metrics.throughput << " ops/sec" << std::endl;
    std::cout << "  Peak Memory:       " << metrics.peak_memory / 1024.0 << " KB" << std::endl;
    std::cout << "  Fragmentation:     " << metrics.fragmentation << "%" << std::endl;
}

void print_concurrency_results(const ConcurrencyMetrics& metrics) {
    std::cout << "\nTest: " << metrics.test_name << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Total Time:     " << metrics.total_time_ms << " ms" << std::endl;
    std::cout << "  Contention:     " << metrics.contention_time_ms << " ms" << std::endl;
    std::cout << "  Throughput:     " << metrics.throughput << " ops/sec" << std::endl;
}

int main() {
    std::cout << "\nMemory Engine Diagnostics Suite - Native Test\n";
    print_separator();

    Engine engine;
    BenchmarkConfig config;
    config.object_size = 256;
    config.object_count = 10000;
    config.iterations = 5;
    config.alignment = 16;

    // Test each allocator
    std::cout << "\n=== Allocator Benchmarks ===\n";
    
    AllocatorType allocators[] = {
        AllocatorType::STANDARD,
        AllocatorType::POOL,
        AllocatorType::STACK,
        AllocatorType::FREELIST
    };

    for (auto type : allocators) {
        engine.set_allocator(type);
        auto metrics = engine.run_benchmark(config);
        print_benchmark_results(metrics);
    }

    // Test concurrency
    std::cout << "\n=== Concurrency Benchmarks ===\n";
    
    ConcurrencyConfig cc;
    cc.thread_count = 4;
    cc.iterations = 1000;
    cc.work_size = 100;

    auto mutex_result = engine.run_concurrency_test(ConcurrencyTest::MUTEX_CONTENTION, cc);
    print_concurrency_results(mutex_result);

    auto atomic_result = engine.run_concurrency_test(ConcurrencyTest::ATOMIC_PERFORMANCE, cc);
    print_concurrency_results(atomic_result);

    print_separator();
    std::cout << "Tests complete.\n" << std::endl;

    return 0;
}
