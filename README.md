# Memory Engine: Diagnostics Suite

A comprehensive memory allocator benchmarking and visualization application built with C++ and WebAssembly for browser-based execution.

## 🚀 Overview

This application provides a sophisticated diagnostic suite for analyzing and benchmarking various memory allocation strategies and concurrency patterns. It compiles C++ code to WebAssembly, allowing it to run directly in modern web browsers with near-native performance.

## ✨ Features

### Memory Allocators
- **Standard Allocator**: Uses default `new`/`delete` operations
- **Pool Allocator**: Pre-allocates fixed-size memory blocks for efficient allocation
- **Stack Allocator**: LIFO-based allocation for temporary memory
- **Free List Allocator**: Manages free memory blocks with various fit strategies

### Concurrency Benchmarks
- **Mutex Contention**: Tests performance under lock contention scenarios
- **Atomic Performance**: Benchmarks lock-free atomic operations
- **Producer-Consumer**: Multi-threaded queue performance testing
- **Thread Creation**: Measures thread spawning overhead

### Visualization
- Real-time memory grid visualization
- Performance metrics dashboard
- Allocation heatmaps
- Contention graphs
- Event logging

## 📁 Project Structure

```
memory_allocator_suite/
├── src/
│   ├── core/
│   │   ├── allocators/
│   │   │   ├── base_allocator.hpp       # Abstract allocator interface
│   │   │   ├── standard_allocator.hpp   # new/delete wrapper
│   │   │   ├── pool_allocator.hpp       # Fixed-size pool allocator
│   │   │   ├── stack_allocator.hpp      # LIFO stack allocator
│   │   │   └── freelist_allocator.hpp   # Free list allocator
│   │   ├── benchmarks/
│   │   │   ├── benchmark_runner.hpp     # Benchmark orchestration
│   │   │   ├── allocation_benchmark.hpp # Memory allocation tests
│   │   │   └── concurrency_benchmark.hpp# Threading tests
│   │   ├── utils/
│   │   │   ├── memory_utils.hpp         # Memory utilities
│   │   │   ├── timer.hpp                # High-resolution timing
│   │   │   └── statistics.hpp           # Statistical analysis
│   │   └── engine.hpp                   # Main engine class
│   ├── bindings/
│   │   └── wasm_bindings.cpp            # Emscripten bindings
│   └── main.cpp                         # Entry point
├── web/
│   ├── index.html                       # Main HTML file
│   ├── css/
│   │   └── styles.css                   # Application styles
│   ├── js/
│   │   ├── app.js                       # Main application logic
│   │   ├── visualizer.js                # Canvas visualization
│   │   ├── metrics.js                   # Metrics display
│   │   └── controls.js                  # UI controls
│   └── assets/
│       └── fonts/                       # Custom fonts
├── docs/
│   ├── API.md                           # API documentation
│   ├── ARCHITECTURE.md                  # System architecture
│   ├── ALLOCATORS.md                    # Allocator implementations
│   └── BENCHMARKS.md                    # Benchmark methodology
├── build/
│   └── (compiled output)
├── CMakeLists.txt                       # CMake build configuration
├── build.bat                            # Windows build script
├── build.sh                             # Unix build script
└── README.md                            # This file
```

## 🛠️ Prerequisites

### For Development
- **Emscripten SDK** (3.1.0+): Compiles C++ to WebAssembly
- **CMake** (3.16+): Build system
- **Modern C++ Compiler**: Supporting C++17 or later
- **Python 3**: For local development server

### For Running
- Modern web browser with WebAssembly support:
  - Chrome 57+
  - Firefox 52+
  - Safari 11+
  - Edge 16+

## 🔧 Installation

### 1. Install Emscripten SDK

```bash
# Clone emsdk repository
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest SDK
./emsdk install latest
./emsdk activate latest

# Set up environment (run this in each new terminal)
source ./emsdk_env.sh  # Linux/macOS
# or
emsdk_env.bat          # Windows
```

### 2. Clone and Build

```bash
# Clone the project
git clone <repository-url>
cd memory_allocator_suite

# Create build directory
mkdir build && cd build

# Configure with CMake (using Emscripten toolchain)
emcmake cmake ..

# Build
emmake make

# Or on Windows with build script
cd ..
build.bat
```

### 3. Run Locally

```bash
# Start local server
cd web
python -m http.server 8080

# Open browser to http://localhost:8080
```

## 📖 Usage

### Basic Usage

1. **Select Allocator**: Choose from the left panel which memory allocator to test
2. **Configure Parameters**:
   - Object Size: Small (64B), Medium (256B), Large (4KB)
   - Alignment: 8, 16, 32, or 64 bytes
   - Object Count: Number of allocations to perform
   - Iterations: Number of benchmark runs

3. **Run Benchmark**: Click "RUN SUITE" to execute the benchmark
4. **Analyze Results**: View real-time metrics and visualization

### Concurrency Testing

1. **Select Concurrency Test**: Choose from mutex, atomic, producer-consumer, or thread creation
2. **Set Thread Count**: Adjust the number of threads
3. **Configure Workload**: Set iteration count and work size
4. **Execute**: Run and observe contention patterns

## 📊 Metrics Explained

| Metric | Description |
|--------|-------------|
| **Latency** | Average time per allocation operation (nanoseconds) |
| **Throughput** | Allocations per second (operations/sec) |
| **Contention** | Time spent waiting for locks (milliseconds) |
| **Pressure Index** | Memory pressure indicator (0-100%) |
| **Fragmentation** | Memory fragmentation percentage |
| **Peak Depth** | Maximum allocation depth reached |
| **Thread Drift** | Thread synchronization variance |

## 🏗️ Architecture

The application follows a modular architecture:

```
┌─────────────────────────────────────────────────────────┐
│                    Web Frontend                         │
│  ┌─────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │Controls │  │ Visualizer  │  │  Metrics Dashboard  │ │
│  └────┬────┘  └──────┬──────┘  └──────────┬──────────┘ │
│       │              │                     │            │
│       └──────────────┼─────────────────────┘            │
│                      │                                  │
│              ┌───────▼───────┐                          │
│              │  JavaScript   │                          │
│              │   Bridge      │                          │
│              └───────┬───────┘                          │
└──────────────────────┼──────────────────────────────────┘
                       │ WebAssembly Interface
┌──────────────────────┼──────────────────────────────────┐
│              ┌───────▼───────┐                          │
│              │    Engine     │         C++ Core         │
│              └───────┬───────┘                          │
│       ┌──────────────┼──────────────┐                   │
│       │              │              │                   │
│  ┌────▼────┐   ┌─────▼─────┐  ┌─────▼─────┐            │
│  │Allocators│  │Benchmarks │  │ Utilities │            │
│  └─────────┘   └───────────┘  └───────────┘            │
└─────────────────────────────────────────────────────────┘
```

## 📜 License

MIT License - See LICENSE file for details.

## 👤 Author

**Bambang Hutagalung**

## 🤝 Contributing

Contributions are welcome! Please read the contributing guidelines before submitting pull requests.

---

*Built with ❤️ using C++, WebAssembly, and modern web technologies*
