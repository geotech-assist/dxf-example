# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DXF Processor is a cross-platform C++ application that parses AutoCAD DXF files and extracts 3D mesh data from 3DFACE entities. The application analyzes triangular mesh data and generates summaries in multiple output formats (JSON, text, CSV).

## Build System and Commands

### Build Commands
The project uses CMake with platform-specific build scripts:

**Unix/Linux/macOS:**
```bash
./scripts/build_unix.sh
```
- Creates both Release (`build/`) and Debug (`build_debug/`) builds
- Automatically detects compiler (Clang++/GCC) and core count
- Executables: `build/bin/dxf_processor` and `build_debug/bin/dxf_processor`

**Windows:**
```bat
scripts\build_windows.bat
```
- Creates Visual Studio solution with Debug and Release configurations
- Executable: `build\bin\Release\dxf_processor.exe` and `build\bin\Debug\dxf_processor.exe`

**Manual CMake build:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)  # or cmake --build . --parallel on Windows
```

### Testing
Tests use Google Test framework and require enabling BUILD_TESTS:

```bash
cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest                          # Run all tests
./tests/test_dxf_processor     # Run specific test executable
make run_tests                 # Custom test target
```

Test data is located in `tests/test_data/` and copied to build directory automatically.

### Running the Application
```bash
# Basic usage
./build/bin/dxf_processor "data/Design Pit.dxf"

# With options
./build/bin/dxf_processor --format json --summarizer detailed --output ./results "data/Design Pit.dxf"

# Show help
./build/bin/dxf_processor --help
```

## Code Architecture

### Core Components

**DXFProcessor Namespace:** All classes are contained within `DXFProcessor` namespace.

**Key Classes:**
1. **DXFReader** (`include/DXFReader.h`, `src/DXFReader.cpp`)
   - Main DXF file parser
   - Extracts 3DFACE entities and converts to triangular mesh data
   - Supports progress callbacks
   - Factory pattern with `DXFReaderFactory`

2. **MeshData** (`include/MeshData.h`)
   - Core data structures: `Point3D`, `Triangle`, `BoundingBox`
   - Container for triangular mesh data with geometric operations
   - Provides bounding box calculation and surface area computation

3. **MeshSummarizer** (`include/MeshSummarizer.h`, `src/MeshSummarizer.cpp`)
   - Analyzes mesh data and generates statistics
   - Two types: "basic" and "detailed" summarizers
   - Factory pattern with `MeshSummarizerFactory`

4. **SummaryWriter** (`include/SummaryWriter.h`, `src/SummaryWriter.cpp`)
   - Writes analysis results to files
   - Supports JSON, text, and CSV formats
   - Configurable timestamp inclusion and pretty printing
   - Factory pattern with `SummaryWriterFactory`

### Design Patterns
- **Factory Pattern**: Used for creating instances of DXFReader, MeshSummarizer, and SummaryWriter
- **Strategy Pattern**: Different summarizer types and output formats
- **RAII**: Smart pointers used throughout for memory management

### Key Data Flow
1. DXFReader parses DXF file → MeshData (triangles)
2. MeshSummarizer analyzes MeshData → MeshSummary (statistics)
3. SummaryWriter outputs MeshSummary → formatted file

### File Organization
```
include/           # Header files
src/              # Implementation files
tests/            # Unit tests with Google Test
  test_data/      # Test DXF files
data/             # Sample DXF files
scripts/          # Platform-specific build scripts
```

## Development Notes

### Compiler Requirements
- C++17 standard required
- Cross-platform: MSVC (Windows), GCC (Linux), Clang (macOS)
- Filesystem library support (automatically linked for older GCC versions)

### Error Handling
Custom exception classes with descriptive prefixes:
- `DXFReaderException`: DXF parsing errors
- `SummaryWriterException`: File writing errors

### Progress Reporting
DXFReader supports progress callbacks for long-running operations, useful for large DXF files with 2900+ entities.

### Memory Management
- Uses smart pointers (`std::unique_ptr`) throughout
- RAII principles followed consistently
- Memory-efficient line-based parsing for large files