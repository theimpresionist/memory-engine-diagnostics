# System Architecture

This document describes the architecture of the Memory Engine Diagnostics Suite.

## High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           WEB BROWSER                                   │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │                      Web Frontend (HTML/CSS/JS)                    │  │
│  │  ┌─────────────┐  ┌──────────────────┐  ┌─────────────────────┐   │  │
│  │  │  Controls   │  │   Visualizer     │  │  Metrics Display    │   │  │
│  │  │  (UI/UX)    │  │  (Canvas/WebGL)  │  │  (Real-time data)   │   │  │
│  │  └──────┬──────┘  └────────┬─────────┘  └──────────┬──────────┘   │  │
│  │         │                  │                       │              │  │
│  │         └──────────────────┼───────────────────────┘              │  │
│  │                            │                                      │  │
│  │                   ┌────────▼────────┐                             │  │
│  │                   │   app.js        │                             │  │
│  │                   │  (Main Logic)   │                             │  │
│  │                   └────────┬────────┘                             │  │
│  └────────────────────────────┼──────────────────────────────────────┘  │
│                               │                                         │
│                    ┌──────────▼──────────┐                              │
│                    │ JavaScript Bindings │                              │
│                    │   (Emscripten)      │                              │
│                    └──────────┬──────────┘                              │
└───────────────────────────────┼─────────────────────────────────────────┘
                                │
                    ════════════╪═══════════════
                      WebAssembly Boundary
                    ════════════╪═══════════════
                                │
┌───────────────────────────────┼─────────────────────────────────────────┐
│                    ┌──────────▼──────────┐                              │
│                    │   WASM Bindings     │        C++ CORE              │
│                    │ (wasm_bindings.cpp) │                              │
│                    └──────────┬──────────┘                              │
│                               │                                         │
│                    ┌──────────▼──────────┐                              │
│                    │      Engine         │                              │
│                    │   (engine.hpp)      │                              │
│                    └──────────┬──────────┘                              │
│           ┌───────────────────┼───────────────────┐                     │
│           │                   │                   │                     │
│  ┌────────▼────────┐ ┌────────▼────────┐ ┌───────▼───────┐             │
│  │   Allocators    │ │   Benchmarks    │ │   Utilities   │             │
│  ├─────────────────┤ ├─────────────────┤ ├───────────────┤             │
│  │ • Standard      │ │ • Benchmark     │ │ • Timer       │             │
│  │ • Pool          │ │   Runner        │ │ • Statistics  │             │
│  │ • Stack         │ │ • Concurrency   │ │ • Memory Utils│             │
│  │ • FreeList      │ │   Benchmark     │ │               │             │
│  └─────────────────┘ └─────────────────┘ └───────────────┘             │
└─────────────────────────────────────────────────────────────────────────┘
```

## Component Details

### 1. Web Frontend

#### HTML Structure (`web/index.html`)
- **Header**: Status display, global metrics
- **Left Sidebar**: Allocator/test selection, configuration
- **Center**: Memory visualization canvas
- **Right Sidebar**: Real-time metrics, graphs

#### CSS Styling (`web/css/styles.css`)
- Dark cyberpunk theme
- CSS custom properties for theming
- Responsive layout with flexbox
- Smooth animations and transitions

#### JavaScript Modules

| Module | Purpose |
|--------|---------|
| `app.js` | Main application controller |
| `visualizer.js` | Canvas-based memory grid rendering |
| `metrics.js` | Real-time metrics display |
| `controls.js` | UI event handling |

### 2. WebAssembly Bridge

#### Binding Layer (`src/bindings/wasm_bindings.cpp`)

Exposes C++ functionality to JavaScript:

```cpp
EMSCRIPTEN_BINDINGS(memory_engine) {
    function("setAllocator", &setAllocator);
    function("runBenchmark", &runBenchmark);
    function("runConcurrencyTest", &runConcurrencyTest);
    function("getStats", &getStats);
    function("getMemoryGrid", &getMemoryGrid);
    function("resetAllocator", &resetAllocator);
}
```

#### Data Flow

```
JavaScript                    WebAssembly
    │                             │
    │  runBenchmark(config)       │
    ├────────────────────────────▶│
    │                             │ Execute C++ benchmark
    │                             │
    │  {metrics object}           │
    ◀────────────────────────────┤
    │                             │
```

### 3. C++ Core Engine

#### Engine Class (`src/core/engine.hpp`)

Central orchestrator managing:
- Allocator lifecycle
- Benchmark execution
- Statistics aggregation

```cpp
class Engine {
    std::map<AllocatorType, std::unique_ptr<BaseAllocator>> m_allocators;
    BenchmarkRunner m_benchmark_runner;
    ConcurrencyBenchmark m_concurrency_bench;
    
public:
    void set_allocator(AllocatorType type);
    BenchmarkMetrics run_benchmark(const BenchmarkConfig& config);
    ConcurrencyMetrics run_concurrency_test(ConcurrencyTest test, 
                                            const ConcurrencyConfig& config);
};
```

### 4. Allocator Hierarchy

```
                    ┌─────────────────┐
                    │  BaseAllocator  │ (Abstract)
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│ StandardAlloc   │ │   PoolAlloc     │ │   StackAlloc    │
└─────────────────┘ └─────────────────┘ └─────────────────┘
         │
         ▼
┌─────────────────┐
│  FreeListAlloc  │
└─────────────────┘
```

#### BaseAllocator Interface

```cpp
class BaseAllocator {
public:
    virtual void* allocate(size_t size, size_t alignment) = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual void reset() = 0;
    virtual bool owns(void* ptr) const = 0;
    
    const AllocationStats& stats() const;
    double fragmentation_percentage() const;
    size_t available() const;
};
```

### 5. Benchmark System

#### Benchmark Runner

Orchestrates benchmark execution:

```cpp
class BenchmarkRunner {
public:
    BenchmarkMetrics run_allocation_benchmark(
        BaseAllocator& allocator,
        const BenchmarkConfig& config
    );
};
```

#### Execution Flow

```
1. Reset Allocator
       │
       ▼
2. Start Timer
       │
       ▼
3. Perform N Allocations
       │
       ▼
4. Record Allocation Time
       │
       ▼
5. Perform N Deallocations
       │
       ▼
6. Record Deallocation Time
       │
       ▼
7. Repeat for M Iterations
       │
       ▼
8. Calculate Statistics
       │
       ▼
9. Return Metrics
```

## Data Structures

### AllocationStats
```cpp
struct AllocationStats {
    size_t total_allocations;
    size_t current_allocations;
    size_t total_bytes_allocated;
    size_t current_bytes_used;
    size_t peak_bytes_used;
    double avg_allocation_time_ns;
    double avg_dealloc_time_ns;
};
```

### BenchmarkMetrics
```cpp
struct BenchmarkMetrics {
    BenchmarkResult allocation_time;    // Statistical summary
    BenchmarkResult deallocation_time;
    double throughput;
    double peak_memory;
    double fragmentation;
    std::string allocator_name;
};
```

### BenchmarkResult
```cpp
struct BenchmarkResult {
    double mean;
    double median;
    double std_dev;
    double min;
    double max;
    double p95;
    double p99;
    size_t sample_count;
};
```

## Build System

### CMake Configuration

```
CMakeLists.txt
    │
    ├── Emscripten Build (WebAssembly)
    │   ├── memory_engine.js
    │   └── memory_engine.wasm
    │
    └── Native Build (Testing)
        └── memory_engine_test
```

### Build Process

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   C++ Source │────▶│  Emscripten  │────▶│   .wasm +    │
│              │     │   Compiler   │     │   .js glue   │
└──────────────┘     └──────────────┘     └──────────────┘
                                                │
                                                ▼
                         ┌─────────────────────────────────┐
                         │  web/wasm/memory_engine.js      │
                         │  web/wasm/memory_engine.wasm    │
                         └─────────────────────────────────┘
```

## Threading Model

### WebAssembly Threading

When compiled with `-s USE_PTHREADS=1`:

```
┌─────────────────────────────────────────────────┐
│                  Main Thread                     │
│  ┌────────────────────────────────────────────┐ │
│  │          JavaScript + WASM Main            │ │
│  └────────────────────────────────────────────┘ │
└─────────────────────┬───────────────────────────┘
                      │ SharedArrayBuffer
          ┌───────────┼───────────┐
          │           │           │
          ▼           ▼           ▼
    ┌──────────┐ ┌──────────┐ ┌──────────┐
    │ Worker 1 │ │ Worker 2 │ │ Worker N │
    │ (pthread)│ │ (pthread)│ │ (pthread)│
    └──────────┘ └──────────┘ └──────────┘
```

### Thread Safety Considerations

| Component | Thread-Safe | Notes |
|-----------|-------------|-------|
| Engine | No | Single-threaded access expected |
| StandardAllocator | Yes | Uses thread-safe new/delete |
| PoolAllocator | No | Requires external locking |
| StackAllocator | No | Single-thread only |
| FreeListAllocator | No | Requires external locking |

## Memory Layout

### Pool Allocator Memory

```
┌─────────────────────────────────────────────────────────────┐
│                     Pool Memory Buffer                       │
├────────┬────────┬────────┬────────┬────────┬────────┬───────┤
│ Block  │ Block  │ Block  │ Block  │ Block  │ Block  │  ...  │
│   0    │   1    │   2    │   3    │   4    │   5    │       │
├────────┴────────┴────────┴────────┴────────┴────────┴───────┤
│ Each block: [Alignment Padding][Fixed Size Data]            │
└─────────────────────────────────────────────────────────────┘
```

### Stack Allocator Memory

```
┌─────────────────────────────────────────────────────────────┐
│                    Stack Memory Buffer                       │
├──────────────────────────────────┬──────────────────────────┤
│           Allocated Region       │      Free Region         │
├────────┬─────────┬────────┬──────┼──────────────────────────┤
│[Header]│ Data A  │[Header]│Data B│                          │
│  (A)   │         │  (B)   │      │                          │
└────────┴─────────┴────────┴──────┴──────────────────────────┘
                                   ▲
                              Current Offset
```

## Extension Points

### Adding a New Allocator

1. Create header in `src/core/allocators/`
2. Inherit from `BaseAllocator`
3. Implement required virtual methods
4. Add to `AllocatorType` enum in `engine.hpp`
5. Initialize in `Engine::initialize_allocators()`
6. Update WASM bindings if needed

### Adding a New Benchmark

1. Add method to `ConcurrencyBenchmark` class
2. Add to `ConcurrencyTest` enum
3. Update `Engine::run_concurrency_test()` switch
4. Update WASM bindings
5. Add UI button in `index.html`
