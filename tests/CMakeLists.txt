# Test configuration for DXF Processor
cmake_minimum_required(VERSION 3.15)

# Include directories from main project
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create a library from the source files (excluding main.cpp) for testing
set(LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/DXFReader.cpp
    ${CMAKE_SOURCE_DIR}/src/MeshSummarizer.cpp
    ${CMAKE_SOURCE_DIR}/src/SummaryWriter.cpp
)

add_library(dxf_processor_lib ${LIB_SOURCES})

# Enable filesystem library for the test library too
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(dxf_processor_lib stdc++fs)
endif()

# Test source files
set(TEST_SOURCES
    test_main.cpp
    test_mesh_data.cpp
    test_dxf_reader.cpp
    test_mesh_summarizer.cpp
    test_summary_writer.cpp
    test_integration.cpp
)

# Create test executable
add_executable(test_dxf_processor ${TEST_SOURCES})

# Link against Google Test, Google Mock, and our library
target_link_libraries(test_dxf_processor 
    dxf_processor_lib
    gtest
    gtest_main
    gmock
    gmock_main
)

# Add compile definitions for test data paths
target_compile_definitions(test_dxf_processor PRIVATE 
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
    MAIN_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(test_dxf_processor)

# Add custom test target
add_custom_target(run_tests
    COMMAND test_dxf_processor
    DEPENDS test_dxf_processor
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running unit tests"
)

# Copy test data files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data/ 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_data/)

# Copy main data directory for integration tests
file(COPY ${CMAKE_SOURCE_DIR}/data/ 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data/)