# API Reference

Complete API documentation for the Memory Engine Diagnostics Suite.

## C++ API

### Engine Class

The main interface for the memory diagnostics engine.

```cpp
#include "core/engine.hpp"
using namespace memory_engine;
```

#### Constructor

```cpp
Engine();
```
Creates a new engine instance with default allocators initialized.

#### Methods

##### set_allocator
```cpp
void set_allocator(AllocatorType type);
```
Sets the active allocator for benchmarking.

**Parameters:**
- `type`: Allocator type enum value

**Allocator Types:**
- `AllocatorType::STANDARD` - Standard new/delete
- `AllocatorType::POOL` - Pool allocator
- `AllocatorType::STACK` - Stack allocator
- `AllocatorType::FREELIST` - Free list allocator

---

##### get_allocator
```cpp
BaseAllocator* get_allocator();
```
Returns pointer to the current active allocator.

**Returns:** Pointer to `BaseAllocator` or nullptr

---

##### run_benchmark
```cpp
BenchmarkMetrics run_benchmark(const BenchmarkConfig& config);
```
Runs allocation benchmark with the current allocator.

**Parameters:**
- `config`: Benchmark configuration

**Returns:** `BenchmarkMetrics` with results

---

##### run_concurrency_test
```cpp
ConcurrencyMetrics run_concurrency_test(
    ConcurrencyTest test, 
    const ConcurrencyConfig& config
);
```
Runs a concurrency benchmark.

**Parameters:**
- `test`: Test type enum
- `config`: Concurrency configuration

**Test Types:**
- `ConcurrencyTest::MUTEX_CONTENTION`
- `ConcurrencyTest::ATOMIC_PERFORMANCE`
- `ConcurrencyTest::PRODUCER_CONSUMER`
- `ConcurrencyTest::THREAD_CREATION`

---

### BaseAllocator Class

Abstract base class for all allocators.

#### Pure Virtual Methods

##### allocate
```cpp
virtual void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
```
Allocates memory.

**Parameters:**
- `size`: Size in bytes
- `alignment`: Alignment requirement (power of 2)

**Returns:** Pointer to allocated memory or nullptr

---

##### deallocate
```cpp
virtual void deallocate(void* ptr) = 0;
```
Deallocates memory.

**Parameters:**
- `ptr`: Pointer to previously allocated memory

---

##### reset
```cpp
virtual void reset() = 0;
```
Resets allocator to initial state, freeing all memory.

---

##### owns
```cpp
virtual bool owns(void* ptr) const = 0;
```
Checks if pointer was allocated by this allocator.

**Parameters:**
- `ptr`: Pointer to check

**Returns:** true if this allocator owns the pointer

---

#### Member Methods

##### stats
```cpp
const AllocationStats& stats() const;
```
Returns current allocation statistics.

---

##### fragmentation_percentage
```cpp
virtual double fragmentation_percentage() const;
```
Returns fragmentation as percentage (0-100).

---

##### available
```cpp
virtual size_t available() const;
```
Returns available free memory in bytes.

---

### PoolAllocator Class

Fixed-size block allocator.

#### Constructor
```cpp
PoolAllocator(
    size_t block_size,      // Size of each block
    size_t block_count,     // Number of blocks
    size_t alignment = alignof(std::max_align_t)
);
```

#### Additional Methods

##### get_allocation_grid
```cpp
std::vector<bool> get_allocation_grid() const;
```
Returns allocation state of each block for visualization.

##### free_blocks / allocated_blocks
```cpp
size_t free_blocks() const;
size_t allocated_blocks() const;
```
Returns count of free/allocated blocks.

---

### StackAllocator Class

LIFO stack-based allocator.

#### Constructor
```cpp
StackAllocator(size_t size, size_t alignment = alignof(std::max_align_t));
```

#### Additional Methods

##### get_marker
```cpp
Marker get_marker() const;
```
Returns current stack position for later rollback.

##### rollback_to_marker
```cpp
void rollback_to_marker(Marker marker);
```
Rolls back all allocations made after marker was obtained.

---

### FreeListAllocator Class

Variable-size allocator with fit policy.

#### Constructor
```cpp
FreeListAllocator(size_t size, FitPolicy policy = FitPolicy::BEST_FIT);
```

**Fit Policies:**
- `FitPolicy::FIRST_FIT`
- `FitPolicy::BEST_FIT`
- `FitPolicy::WORST_FIT`

#### Additional Methods

##### set_policy
```cpp
void set_policy(FitPolicy policy);
```
Changes the allocation fit policy.

##### largest_free_block
```cpp
size_t largest_free_block() const;
```
Returns size of largest contiguous free region.

---

### Configuration Structures

#### BenchmarkConfig
```cpp
struct BenchmarkConfig {
    size_t object_size = 256;      // Size of each object
    size_t object_count = 10000;   // Number of allocations
    size_t iterations = 10;        // Benchmark iterations
    size_t alignment = 8;          // Memory alignment
    size_t thread_count = 1;       // Threads (future use)
    bool randomize_order = false;  // Randomize deallocation
};
```

#### ConcurrencyConfig
```cpp
struct ConcurrencyConfig {
    size_t thread_count = 4;   // Number of threads
    size_t iterations = 1000;  // Operations per thread
    size_t work_size = 100;    // Work units per iteration
};
```

---

### Result Structures

#### AllocationStats
```cpp
struct AllocationStats {
    size_t total_allocations;      // Total allocations made
    size_t total_deallocations;    // Total deallocations made
    size_t current_allocations;    // Currently active
    size_t total_bytes_allocated;  // Total bytes ever allocated
    size_t current_bytes_used;     // Current memory in use
    size_t peak_bytes_used;        // Peak memory usage
    size_t fragmentation_bytes;    // Estimated fragmentation
    double avg_allocation_time_ns; // Average alloc time
    double avg_dealloc_time_ns;    // Average dealloc time
};
```

#### BenchmarkResult
```cpp
struct BenchmarkResult {
    double mean;         // Average value
    double median;       // Median value
    double std_dev;      // Standard deviation
    double min;          // Minimum value
    double max;          // Maximum value
    double p95;          // 95th percentile
    double p99;          // 99th percentile
    size_t sample_count; // Number of samples
};
```

#### BenchmarkMetrics
```cpp
struct BenchmarkMetrics {
    BenchmarkResult allocation_time;   // Allocation timing stats
    BenchmarkResult deallocation_time; // Deallocation timing stats
    double throughput;                 // Operations per second
    double peak_memory;                // Peak memory used
    double fragmentation;              // Fragmentation percentage
    std::string allocator_name;        // Name of allocator
};
```

#### ConcurrencyMetrics
```cpp
struct ConcurrencyMetrics {
    double total_time_ms;      // Total benchmark time
    double contention_time_ms; // Time spent waiting for locks
    double throughput;         // Operations per second
    double thread_efficiency;  // Parallel efficiency
    std::string test_name;     // Name of test
};
```

---

## JavaScript API

### Module Loading

```javascript
// The WASM module is loaded automatically
// Access via global Module object or import
```

### Functions

#### setAllocator
```javascript
Module.setAllocator(type: number): number
```
Sets the active allocator.

**Parameters:**
- `type`: 0=Standard, 1=Pool, 2=Stack, 3=FreeList

**Returns:** 0 on success

---

#### runBenchmark
```javascript
Module.runBenchmark(
    objectSize: number,
    objectCount: number,
    iterations: number,
    alignment: number
): Object
```
Runs allocation benchmark.

**Returns:**
```javascript
{
    allocatorName: string,
    meanAllocTime: number,      // nanoseconds
    medianAllocTime: number,
    minAllocTime: number,
    maxAllocTime: number,
    stdDevAllocTime: number,
    meanDeallocTime: number,
    throughput: number,         // ops/sec
    peakMemory: number,         // bytes
    fragmentation: number       // percentage
}
```

---

#### runConcurrencyTest
```javascript
Module.runConcurrencyTest(
    testType: number,
    threadCount: number,
    iterations: number,
    workSize: number
): Object
```
Runs concurrency benchmark.

**Parameters:**
- `testType`: 0=Mutex, 1=Atomic, 2=Producer-Consumer, 3=Thread Creation

**Returns:**
```javascript
{
    testName: string,
    totalTimeMs: number,
    contentionTimeMs: number,
    throughput: number,
    threadEfficiency: number
}
```

---

#### getStats
```javascript
Module.getStats(): Object
```
Gets current allocation statistics.

**Returns:**
```javascript
{
    totalAllocations: number,
    currentAllocations: number,
    totalBytesAllocated: number,
    currentBytesUsed: number,
    peakBytesUsed: number,
    avgAllocationTime: number,
    avgDeallocTime: number
}
```

---

#### getMemoryGrid
```javascript
Module.getMemoryGrid(): Array<boolean>
```
Gets pool allocator block states.

**Returns:** Array of booleans (true = allocated)

---

#### resetAllocator
```javascript
Module.resetAllocator(): void
```
Resets the current allocator to initial state.

---

## Usage Examples

### C++ Native Usage

```cpp
#include "core/engine.hpp"

int main() {
    memory_engine::Engine engine;
    
    // Configure benchmark
    memory_engine::BenchmarkConfig config;
    config.object_size = 256;
    config.object_count = 10000;
    config.iterations = 5;
    
    // Test pool allocator
    engine.set_allocator(memory_engine::AllocatorType::POOL);
    auto results = engine.run_benchmark(config);
    
    std::cout << "Mean allocation time: " 
              << results.allocation_time.mean << " ns\n";
    std::cout << "Throughput: " 
              << results.throughput << " ops/sec\n";
    
    return 0;
}
```

### JavaScript Usage

```javascript
// After WASM module loads
async function runTest() {
    // Set allocator to Pool
    Module.setAllocator(1);
    
    // Run benchmark
    const results = Module.runBenchmark(256, 10000, 5, 16);
    
    console.log(`Allocator: ${results.allocatorName}`);
    console.log(`Mean alloc time: ${results.meanAllocTime.toFixed(2)} ns`);
    console.log(`Throughput: ${(results.throughput/1000).toFixed(1)}k ops/sec`);
    
    // Get memory visualization
    const grid = Module.getMemoryGrid();
    visualizeGrid(grid);
}
```
