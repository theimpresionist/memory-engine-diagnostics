/**
 * @file wasm_bindings.cpp
 * @brief Emscripten WebAssembly bindings
 */

#include "../core/engine.hpp"
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;
using namespace memory_engine;

// Global engine instance
static Engine g_engine;

// JavaScript-accessible functions
int setAllocator(int type) {
    g_engine.set_allocator(static_cast<AllocatorType>(type));
    return 0;
}

val runBenchmark(int objectSize, int objectCount, int iterations, int alignment) {
    BenchmarkConfig config;
    config.object_size = objectSize;
    config.object_count = objectCount;
    config.iterations = iterations;
    config.alignment = alignment;

    auto metrics = g_engine.run_benchmark(config);

    val result = val::object();
    result.set("allocatorName", metrics.allocator_name);
    result.set("meanAllocTime", metrics.allocation_time.mean);
    result.set("medianAllocTime", metrics.allocation_time.median);
    result.set("minAllocTime", metrics.allocation_time.min);
    result.set("maxAllocTime", metrics.allocation_time.max);
    result.set("stdDevAllocTime", metrics.allocation_time.std_dev);
    result.set("meanDeallocTime", metrics.deallocation_time.mean);
    result.set("throughput", metrics.throughput);
    result.set("peakMemory", metrics.peak_memory);
    result.set("fragmentation", metrics.fragmentation);
    
    return result;
}

val runConcurrencyTest(int testType, int threadCount, int iterations, int workSize) {
    ConcurrencyConfig config;
    config.thread_count = threadCount;
    config.iterations = iterations;
    config.work_size = workSize;

    auto metrics = g_engine.run_concurrency_test(static_cast<ConcurrencyTest>(testType), config);

    val result = val::object();
    result.set("testName", metrics.test_name);
    result.set("totalTimeMs", metrics.total_time_ms);
    result.set("contentionTimeMs", metrics.contention_time_ms);
    result.set("throughput", metrics.throughput);
    result.set("threadEfficiency", metrics.thread_efficiency);
    
    return result;
}

val getStats() {
    auto stats = g_engine.get_stats();
    
    val result = val::object();
    result.set("totalAllocations", static_cast<double>(stats.total_allocations));
    result.set("currentAllocations", static_cast<double>(stats.current_allocations));
    result.set("totalBytesAllocated", static_cast<double>(stats.total_bytes_allocated));
    result.set("currentBytesUsed", static_cast<double>(stats.current_bytes_used));
    result.set("peakBytesUsed", static_cast<double>(stats.peak_bytes_used));
    result.set("avgAllocationTime", stats.avg_allocation_time_ns);
    result.set("avgDeallocTime", stats.avg_dealloc_time_ns);
    
    return result;
}

val getMemoryGrid() {
    auto grid = g_engine.get_memory_grid();
    
    val result = val::array();
    for (size_t i = 0; i < grid.size(); ++i) {
        result.set(i, grid[i]);
    }
    
    return result;
}

void resetAllocator() {
    g_engine.reset_current_allocator();
}

// Emscripten bindings
EMSCRIPTEN_BINDINGS(memory_engine) {
    function("setAllocator", &setAllocator);
    function("runBenchmark", &runBenchmark);
    function("runConcurrencyTest", &runConcurrencyTest);
    function("getStats", &getStats);
    function("getMemoryGrid", &getMemoryGrid);
    function("resetAllocator", &resetAllocator);
}
