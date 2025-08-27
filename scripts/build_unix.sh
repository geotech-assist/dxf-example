#!/bin/bash
# Unix build script for DXF Processor (Linux/macOS)
# Supports GCC, Clang, and other Unix compilers

set -e  # Exit on any error

echo "DXF Processor - Unix Build Script"
echo "=================================="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Detect compiler
if command -v clang++ >/dev/null 2>&1; then
    COMPILER="Clang++"
    export CXX=clang++
    export CC=clang
elif command -v g++ >/dev/null 2>&1; then
    COMPILER="GCC"
    export CXX=g++
    export CC=gcc
else
    echo "Warning: No suitable C++ compiler found. Trying default..."
    COMPILER="Default"
fi

echo "Detected compiler: $COMPILER"

# Detect number of cores for parallel build
if command -v nproc >/dev/null 2>&1; then
    CORES=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4
fi

echo "Building with $CORES cores"

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

echo
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

echo
echo "Building project (Release)..."
make -j$CORES

# Create debug build in separate directory
cd ..
if [ ! -d "build_debug" ]; then
    echo "Creating debug build directory..."
    mkdir build_debug
fi

cd build_debug

echo
echo "Configuring debug build..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"

echo "Building project (Debug)..."
make -j$CORES

cd ..

echo
echo "Build completed successfully!"
echo
echo "Executables are available at:"
echo "  Release: build/bin/dxf_processor"
echo "  Debug:   build_debug/bin/dxf_processor"
echo
echo "To test the application, run:"
echo "  ./build/bin/dxf_processor \"data/Design Pit.dxf\""
echo
echo "Or with options:"
echo "  ./build/bin/dxf_processor --format json --summarizer detailed \"data/Design Pit.dxf\""

# Make the executable runnable
chmod +x build/bin/dxf_processor 2>/dev/null || true
chmod +x build_debug/bin/dxf_processor 2>/dev/null || true