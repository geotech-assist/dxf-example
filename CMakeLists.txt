cmake_minimum_required(VERSION 3.15)
project(dxf_processor VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    # Set startup project for Visual Studio
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT dxf_processor)
    # Enable UTF-8 source code and execution character set
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Create include directory structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/DXFReader.cpp
    src/MeshSummarizer.cpp
    src/SummaryWriter.cpp
)

# Header files
set(HEADERS
    include/DXFReader.h
    include/MeshData.h
    include/MeshSummarizer.h
    include/SummaryWriter.h
)

# Create executable
add_executable(dxf_processor ${SOURCES} ${HEADERS})

# Enable filesystem library (needed for some older compilers)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(dxf_processor stdc++fs)
endif()

# Configure for Visual Studio solution
if(MSVC)
    # Organize files in Visual Studio solution
    source_group("Header Files" FILES ${HEADERS})
    source_group("Source Files" FILES ${SOURCES})
    
    # Set different configurations
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()

# Add tests (optional, can be enabled with -DBUILD_TESTS=ON)
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    # Enable testing and find packages
    enable_testing()
    include(FetchContent)
    
    # Download and configure Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    
    # Add tests directory
    add_subdirectory(tests)
endif()

# Install rules
install(TARGETS dxf_processor
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Print configuration summary
message(STATUS "DXF Processor Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Generator: ${CMAKE_GENERATOR}")