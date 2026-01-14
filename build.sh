#!/bin/bash
# Memory Engine Build Script for Linux/macOS
# Requires Emscripten SDK to be installed and activated

echo "========================================"
echo "Memory Engine Diagnostics Suite Builder"
echo "========================================"
echo

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "ERROR: Emscripten not found in PATH."
    echo "Please install and activate emsdk first:"
    echo "  1. git clone https://github.com/emscripten-core/emsdk.git"
    echo "  2. cd emsdk"
    echo "  3. ./emsdk install latest"
    echo "  4. ./emsdk activate latest"
    echo "  5. source ./emsdk_env.sh"
    exit 1
fi

# Create directories
mkdir -p build
mkdir -p web/wasm

echo "[1/4] Configuring project..."
cd build
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    cd ..
    exit 1
fi

echo
echo "[2/4] Building WebAssembly module..."
emmake make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed."
    cd ..
    exit 1
fi

cd ..

echo
echo "[3/4] Copying assets..."
if [ ! -f web/wasm/memory_engine.js ]; then
    echo "WARNING: WASM output not found. Build may have failed."
fi

echo
echo "[4/4] Build complete!"
echo
echo "To run the application:"
echo "  cd web"
echo "  python3 -m http.server 8080"
echo "  Open http://localhost:8080 in your browser"
echo
echo "========================================"
