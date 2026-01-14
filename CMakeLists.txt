cmake_minimum_required(VERSION 3.16)
project(MemoryEngineDiagnostics VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/bindings/wasm_bindings.cpp
)

# Header files (for IDE support)
set(HEADERS
    src/core/engine.hpp
    src/core/allocators/base_allocator.hpp
    src/core/allocators/standard_allocator.hpp
    src/core/allocators/pool_allocator.hpp
    src/core/allocators/stack_allocator.hpp
    src/core/allocators/freelist_allocator.hpp
    src/core/benchmarks/benchmark_runner.hpp
    src/core/benchmarks/concurrency_benchmark.hpp
    src/core/utils/timer.hpp
    src/core/utils/statistics.hpp
    src/core/utils/memory_utils.hpp
)

# Check if we're using Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building with Emscripten for WebAssembly")
    
    add_executable(memory_engine ${SOURCES} ${HEADERS})
    
    # Emscripten-specific flags
    set_target_properties(memory_engine PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "-s WASM=1 \
                    -s MODULARIZE=1 \
                    -s EXPORT_NAME='MemoryEngineModule' \
                    -s ALLOW_MEMORY_GROWTH=1 \
                    -s MAXIMUM_MEMORY=1GB \
                    -s USE_PTHREADS=1 \
                    -s PTHREAD_POOL_SIZE=navigator.hardwareConcurrency \
                    --bind \
                    -O3 \
                    -s ASSERTIONS=0 \
                    -s DISABLE_EXCEPTION_CATCHING=1"
    )
    
    # Output to web directory
    set_target_properties(memory_engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/web/wasm"
    )
    
else()
    message(STATUS "Building native test executable")
    
    # Native build for testing
    add_executable(memory_engine_test
        src/main.cpp
        ${HEADERS}
    )
    
    target_include_directories(memory_engine_test PRIVATE src)
    
    # Threading support
    find_package(Threads REQUIRED)
    target_link_libraries(memory_engine_test Threads::Threads)
    
    # Enable optimizations for release
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(memory_engine_test PRIVATE -O3)
    endif()
endif()

# Include directories
target_include_directories(memory_engine PRIVATE src)

# Installation
install(DIRECTORY web/ DESTINATION share/memory_engine/web)
install(FILES README.md DESTINATION share/memory_engine)
