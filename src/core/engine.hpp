/**
 * @file engine.hpp
 * @brief Main memory diagnostics engine
 */

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "allocators/base_allocator.hpp"
#include "allocators/standard_allocator.hpp"
#include "allocators/pool_allocator.hpp"
#include "allocators/stack_allocator.hpp"
#include "allocators/freelist_allocator.hpp"
#include "benchmarks/benchmark_runner.hpp"
#include "benchmarks/concurrency_benchmark.hpp"
#include "utils/memory_utils.hpp"
#include <memory>
#include <map>

namespace memory_engine {

enum class AllocatorType {
    STANDARD,
    POOL,
    STACK,
    FREELIST
};

enum class ConcurrencyTest {
    MUTEX_CONTENTION,
    ATOMIC_PERFORMANCE,
    PRODUCER_CONSUMER,
    THREAD_CREATION
};

class Engine {
public:
    Engine() : m_current_allocator(AllocatorType::STANDARD) {
        initialize_allocators();
    }

    void set_allocator(AllocatorType type) {
        m_current_allocator = type;
    }

    BaseAllocator* get_allocator() {
        return m_allocators[m_current_allocator].get();
    }

    BenchmarkMetrics run_benchmark(const BenchmarkConfig& config) {
        auto* allocator = get_allocator();
        if (!allocator) return {};
        return m_benchmark_runner.run_allocation_benchmark(*allocator, config);
    }

    ConcurrencyMetrics run_concurrency_test(ConcurrencyTest test, const ConcurrencyConfig& config) {
        switch (test) {
            case ConcurrencyTest::MUTEX_CONTENTION:
                return m_concurrency_bench.run_mutex_contention(config);
            case ConcurrencyTest::ATOMIC_PERFORMANCE:
                return m_concurrency_bench.run_atomic_performance(config);
            case ConcurrencyTest::PRODUCER_CONSUMER:
                return m_concurrency_bench.run_producer_consumer(config);
            case ConcurrencyTest::THREAD_CREATION:
                return m_concurrency_bench.run_thread_creation(config);
        }
        return {};
    }

    void set_progress_callback(BenchmarkRunner::ProgressCallback callback) {
        m_benchmark_runner.set_progress_callback(callback);
    }

    void reset_current_allocator() {
        auto* allocator = get_allocator();
        if (allocator) allocator->reset();
    }

    AllocationStats get_stats() {
        auto* allocator = get_allocator();
        return allocator ? allocator->stats() : AllocationStats{};
    }

    std::vector<bool> get_memory_grid() {
        if (m_current_allocator == AllocatorType::POOL) {
            auto* pool = dynamic_cast<PoolAllocator*>(get_allocator());
            if (pool) return pool->get_allocation_grid();
        }
        return {};
    }

private:
    void initialize_allocators() {
        m_allocators[AllocatorType::STANDARD] = std::make_unique<StandardAllocator>();
        m_allocators[AllocatorType::POOL] = std::make_unique<PoolAllocator>(4096, 10000);
        m_allocators[AllocatorType::STACK] = std::make_unique<StackAllocator>(MemoryUtils::MB(16));
        m_allocators[AllocatorType::FREELIST] = std::make_unique<FreeListAllocator>(MemoryUtils::MB(16));
    }

    std::map<AllocatorType, std::unique_ptr<BaseAllocator>> m_allocators;
    AllocatorType m_current_allocator;
    BenchmarkRunner m_benchmark_runner;
    ConcurrencyBenchmark m_concurrency_bench;
};

} // namespace memory_engine

#endif // ENGINE_HPP
