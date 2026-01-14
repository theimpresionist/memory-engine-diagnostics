@echo off
REM Memory Engine Build Script for Windows
REM Requires Emscripten SDK to be installed and activated

echo ========================================
echo Memory Engine Diagnostics Suite Builder
echo ========================================
echo.

REM Check if emcc is available
where emcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Emscripten not found in PATH.
    echo Please install and activate emsdk first:
    echo   1. git clone https://github.com/emscripten-core/emsdk.git
    echo   2. cd emsdk
    echo   3. emsdk install latest
    echo   4. emsdk activate latest
    echo   5. emsdk_env.bat
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
if not exist web\wasm mkdir web\wasm

echo [1/4] Configuring project...
cd build
call emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed.
    cd ..
    exit /b 1
)

echo.
echo [2/4] Building WebAssembly module...
call emmake make -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed.
    cd ..
    exit /b 1
)

cd ..

echo.
echo [3/4] Copying assets...
if not exist web\wasm\memory_engine.js (
    echo WARNING: WASM output not found. Build may have failed.
)

echo.
echo [4/4] Build complete!
echo.
echo To run the application:
echo   cd web
echo   python -m http.server 8080
echo   Open http://localhost:8080 in your browser
echo.
echo ========================================
