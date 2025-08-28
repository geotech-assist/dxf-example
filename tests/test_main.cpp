/**
 * @file test_main.cpp
 * @brief Main entry point for DXF Processor unit tests
 * 
 * This file serves as the test runner for the comprehensive test suite
 * for the DXF Processor library using Google Test framework.
 */

#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char **argv) {
    std::cout << "Running DXF Processor Test Suite\n";
    std::cout << "================================\n\n";
    
    ::testing::InitGoogleTest(&argc, argv);
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\nTest Suite Complete\n";
    return result;
}