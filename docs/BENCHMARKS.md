# Concurrency Benchmarks

This document describes the concurrency benchmarks in the Memory Engine Diagnostics Suite.

## Overview

The concurrency benchmarks measure multi-threading performance characteristics relevant to memory allocation scenarios. They help identify contention issues and threading overhead.

## Benchmark Types

### 1. Mutex Contention

Tests the performance impact of mutex-based synchronization under various contention levels.

#### What It Measures
- Lock acquisition time
- Contention overhead
- Throughput under synchronized access

#### Implementation
```cpp
std::mutex mtx;
for (size_t i = 0; i < iterations; ++i) {
    std::lock_guard<std::mutex> lock(mtx);
    // Critical section work
}
```

#### Metrics
- **Total Time**: Wall clock time for all threads
- **Contention Time**: Cumulative time threads spent waiting for locks
- **Throughput**: Operations per second

#### Interpreting Results

| Contention Level | Typical Impact |
|-----------------|----------------|
| < 1ms | Excellent - minimal contention |
| 1-10ms | Good - acceptable contention |
| 10-100ms | Warning - consider optimization |
| > 100ms | Critical - severe bottleneck |

---

### 2. Atomic Performance

Benchmarks lock-free atomic operations for comparison with mutex-based synchronization.

#### What It Measures
- Atomic operation overhead
- Cache line contention
- Memory ordering costs

#### Implementation
```cpp
std::atomic<size_t> counter{0};
for (size_t i = 0; i < iterations; ++i) {
    counter.fetch_add(1, std::memory_order_relaxed);
    // Additional atomic operations
}
```

#### Memory Orderings Tested
- `memory_order_relaxed`: Fastest, no synchronization
- `memory_order_seq_cst`: Strongest, full ordering

#### Expected Performance
Atomic operations are typically 10-100x faster than mutex operations for simple counters, but scale differently with contention.

---

### 3. Producer-Consumer

Tests classic multi-threaded producer-consumer patterns with shared queues.

#### What It Measures
- Queue throughput
- Thread synchronization efficiency
- Condition variable performance

#### Implementation
```
┌──────────────┐     ┌─────────────────┐     ┌──────────────┐
│  Producer 1  │────▶│                 │────▶│  Consumer 1  │
│  Producer 2  │────▶│  Shared Queue   │────▶│  Consumer 2  │
│  Producer N  │────▶│                 │────▶│  Consumer N  │
└──────────────┘     └─────────────────┘     └──────────────┘
```

#### Configuration
- Equal producers and consumers (thread_count / 2 each)
- Bounded queue with condition variable notification
- Timeout-based waiting to prevent deadlock

#### Use Cases
This pattern is common in:
- Memory allocator request queues
- Work stealing schedulers
- I/O completion handlers

---

### 4. Thread Creation

Measures the overhead of thread creation and destruction.

#### What It Measures
- Thread spawn latency
- Join synchronization cost
- System scheduling overhead

#### Implementation
```cpp
for (size_t i = 0; i < iterations; ++i) {
    std::vector<std::thread> threads;
    for (size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([]() { /* minimal work */ });
    }
    for (auto& t : threads) t.join();
}
```

#### Why It Matters
Thread creation is expensive. This benchmark helps decide:
- When to use thread pools
- Optimal worker thread count
- Trade-offs between task granularity and threading overhead

#### Typical Results

| Platform | Thread Creation Time |
|----------|---------------------|
| Windows | ~20-50 μs |
| Linux | ~10-30 μs |
| macOS | ~15-40 μs |

---

## Configuration Parameters

```cpp
struct ConcurrencyConfig {
    size_t thread_count = 4;   // Number of threads to use
    size_t iterations = 1000;  // Operations per thread
    size_t work_size = 100;    // Simulated work units
};
```

### Parameter Guidelines

| Parameter | Low | Medium | High |
|-----------|-----|--------|------|
| thread_count | 2-4 | 8-16 | 32+ |
| iterations | 100 | 1000 | 10000 |
| work_size | 10 | 100 | 1000 |

---

## Metrics Explained

### Total Time (ms)
Wall clock time from start to finish of the benchmark. Includes all thread creation, execution, and joining.

### Contention Time (ms)
Cumulative time spent waiting for synchronization primitives. High values indicate:
- Too many threads competing for resources
- Critical sections that are too long
- Suboptimal lock granularity

### Throughput (ops/sec)
Number of operations completed per second. Higher is better. Calculated as:
```
throughput = total_operations / (total_time_ns / 1,000,000,000)
```

### Thread Efficiency
Ratio of actual work to potential parallel work:
```
efficiency = actual_throughput / (single_thread_throughput × thread_count)
```

| Efficiency | Interpretation |
|------------|----------------|
| > 80% | Excellent scaling |
| 50-80% | Good scaling |
| 20-50% | Poor scaling |
| < 20% | Severe contention |

---

## Best Practices for Memory Allocators

### Reducing Contention

1. **Thread-Local Caches**
   ```cpp
   thread_local PoolAllocator local_pool(4096, 1000);
   ```

2. **Lock Striping**
   ```cpp
   std::array<std::mutex, 16> stripe_mutexes;
   auto& mutex = stripe_mutexes[hash(ptr) % 16];
   ```

3. **Lock-Free Data Structures**
   - Use atomic free lists
   - Compare-and-swap for pointer updates

### Optimal Thread Count

For memory allocation workloads:
- **CPU-bound**: threads = physical_cores
- **Mixed workloads**: threads = logical_cores
- **High contention**: threads = physical_cores / 2

---

## Visualization

The diagnostics suite visualizes contention in real-time:

```
CONTENTION GRAPH
    ▲
2ms │    ╱╲
    │   ╱  ╲   ╱╲
1ms │  ╱    ╲ ╱  ╲
    │ ╱      ╳    ╲
0ms │╱            ╲___
    └─────────────────▶
         Time
```

- **Red peaks**: High contention periods
- **Green baseline**: Normal operation
- **Yellow**: Warning threshold

---

## Troubleshooting

### High Contention
- Reduce critical section size
- Use finer-grained locking
- Consider lock-free alternatives
- Reduce thread count

### Low Throughput
- Check for false sharing (cache line conflicts)
- Profile for memory allocation within critical sections
- Consider batch operations

### Thread Drift
High variance in thread completion times may indicate:
- Unbalanced workloads
- NUMA effects
- Priority inversion
