/**
 * @file test_integration.cpp
 * @brief Integration tests for complete DXF processing pipeline
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "DXFReader.h"
#include "MeshSummarizer.h"
#include "SummaryWriter.h"
#include <filesystem>
#include <memory>
#include <chrono>

using namespace DXFProcessor;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        mainDataDir = MAIN_DATA_DIR;
        testOutputDir = "integration_test_output";
        
        // Ensure output directory exists
        std::filesystem::create_directories(testOutputDir);
        
        designPitPath = mainDataDir + "/Design Pit.dxf";
        
        // Verify test data exists
        if (!std::filesystem::exists(designPitPath)) {
            GTEST_SKIP() << "Design Pit.dxf not found, skipping integration tests";
        }
    }
    
    void TearDown() override {
        // Clean up test output
        if (std::filesystem::exists(testOutputDir)) {
            std::filesystem::remove_all(testOutputDir);
        }
    }
    
    std::string mainDataDir;
    std::string testOutputDir;
    std::string designPitPath;
};

TEST_F(IntegrationTest, CompleteProcessingPipeline) {
    // Step 1: Read DXF file
    auto reader = DXFReaderFactory::createReader();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    auto meshData = reader->readFile(designPitPath);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "DXF parsing took: " << duration.count() << " ms" << std::endl;
    
    // Verify expected results from Design Pit.dxf
    EXPECT_EQ(meshData->getTriangleCount(), 2929);
    EXPECT_EQ(reader->getLastEntityCount(), 2929);
    
    // Step 2: Analyze mesh
    auto summarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Detailed);
    auto summary = summarizer->summarize(*meshData);
    
    // Verify analysis results
    EXPECT_EQ(summary.triangleCount, 2929);
    EXPECT_NEAR(summary.totalSurfaceArea, 141519.89, 1.0);
    
    // Step 3: Write output
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(true);
    writer->setPrettyPrint(true);
    
    std::string outputPath = writer->writeToFile(summary, "integration_test");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    EXPECT_FALSE(outputPath.empty());
}

TEST_F(IntegrationTest, DesignPitDXFSpecificValidation) {
    auto reader = DXFReaderFactory::createReader();
    auto meshData = reader->readFile(designPitPath);
    
    // Validate specific properties of Design Pit.dxf
    EXPECT_EQ(meshData->getTriangleCount(), 2929);
    
    // Test bounding box
    BoundingBox bbox = meshData->getBoundingBox();
    
    // Expected bounding box from previous testing
    EXPECT_NEAR(bbox.min.x, -773.0, 1.0);
    EXPECT_NEAR(bbox.min.y, 668.72, 1.0);
    EXPECT_NEAR(bbox.min.z, 196.74, 1.0);
    
    EXPECT_NEAR(bbox.max.x, -296.0, 1.0);
    EXPECT_NEAR(bbox.max.y, 1001.22, 1.0);
    EXPECT_NEAR(bbox.max.z, 381.0, 1.0);
    
    // Test dimensions
    Point3D size = bbox.size();
    EXPECT_NEAR(size.x, 477.0, 1.0);
    EXPECT_NEAR(size.y, 332.5, 1.0);
    EXPECT_NEAR(size.z, 184.26, 1.0);
    
    // Test surface area
    double totalArea = meshData->getTotalSurfaceArea();
    EXPECT_NEAR(totalArea, 141519.89, 10.0);
}

TEST_F(IntegrationTest, ProgressReporting) {
    auto reader = DXFReaderFactory::createReader();
    
    std::vector<double> progressValues;
    reader->setProgressCallback([&progressValues](double progress) {
        progressValues.push_back(progress);
    });
    
    auto meshData = reader->readFile(designPitPath);
    
    EXPECT_FALSE(progressValues.empty());
    EXPECT_DOUBLE_EQ(progressValues.back(), 1.0);
    
    // Progress should be monotonically increasing
    for (size_t i = 1; i < progressValues.size(); ++i) {
        EXPECT_GE(progressValues[i], progressValues[i-1]);
    }
}

TEST_F(IntegrationTest, BasicVsDetailedSummarizer) {
    auto reader = DXFReaderFactory::createReader();
    auto meshData = reader->readFile(designPitPath);
    
    auto basicSummarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Basic);
    auto detailedSummarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Detailed);
    
    auto basicSummary = basicSummarizer->summarize(*meshData);
    auto detailedSummary = detailedSummarizer->summarize(*meshData);
    
    // Basic properties should be the same
    EXPECT_EQ(basicSummary.triangleCount, detailedSummary.triangleCount);
    EXPECT_DOUBLE_EQ(basicSummary.totalSurfaceArea, detailedSummary.totalSurfaceArea);
    EXPECT_TRUE(basicSummary.boundingBox.min == detailedSummary.boundingBox.min);
    EXPECT_TRUE(basicSummary.boundingBox.max == detailedSummary.boundingBox.max);
    
    // Detailed should have more custom fields
    EXPECT_GT(detailedSummary.customFields.size(), basicSummary.customFields.size());
    
    // Check for detailed-specific fields
    EXPECT_NE(detailedSummary.getCustomField("volume_estimate"), "");
    EXPECT_NE(detailedSummary.getCustomField("min_triangle_area"), "");
    EXPECT_NE(detailedSummary.getCustomField("max_triangle_area"), "");
}

TEST_F(IntegrationTest, AllOutputFormats) {
    auto reader = DXFReaderFactory::createReader();
    auto meshData = reader->readFile(designPitPath);
    
    auto summarizer = MeshSummarizerFactory::create();
    auto summary = summarizer->summarize(*meshData);
    
    // Test all output formats
    auto jsonWriter = SummaryWriterFactory::create("json", testOutputDir);
    auto textWriter = SummaryWriterFactory::create("text", testOutputDir);
    auto csvWriter = SummaryWriterFactory::create("csv", testOutputDir);
    
    jsonWriter->setIncludeTimestamp(false);
    textWriter->setIncludeTimestamp(false);
    csvWriter->setIncludeTimestamp(false);
    
    std::string jsonPath = jsonWriter->writeToFile(summary, "design_pit");
    std::string textPath = textWriter->writeToFile(summary, "design_pit");
    std::string csvPath = csvWriter->writeToFile(summary, "design_pit");
    
    EXPECT_TRUE(std::filesystem::exists(jsonPath));
    EXPECT_TRUE(std::filesystem::exists(textPath));
    EXPECT_TRUE(std::filesystem::exists(csvPath));
    
    // Verify file sizes (should contain substantial content)
    EXPECT_GT(std::filesystem::file_size(jsonPath), 500);
    EXPECT_GT(std::filesystem::file_size(textPath), 300);
    EXPECT_GT(std::filesystem::file_size(csvPath), 200);
}

TEST_F(IntegrationTest, PerformanceBenchmark) {
    auto reader = DXFReaderFactory::createReader();
    
    // Measure parsing performance
    auto startParse = std::chrono::high_resolution_clock::now();
    auto meshData = reader->readFile(designPitPath);
    auto endParse = std::chrono::high_resolution_clock::now();
    
    auto parseTime = std::chrono::duration_cast<std::chrono::milliseconds>(endParse - startParse);
    
    // Measure analysis performance
    auto summarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Detailed);
    
    auto startAnalyze = std::chrono::high_resolution_clock::now();
    auto summary = summarizer->summarize(*meshData);
    auto endAnalyze = std::chrono::high_resolution_clock::now();
    
    auto analyzeTime = std::chrono::duration_cast<std::chrono::milliseconds>(endAnalyze - startAnalyze);
    
    // Measure writing performance
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);
    
    auto startWrite = std::chrono::high_resolution_clock::now();
    std::string outputPath = writer->writeToFile(summary, "benchmark");
    auto endWrite = std::chrono::high_resolution_clock::now();
    
    auto writeTime = std::chrono::duration_cast<std::chrono::milliseconds>(endWrite - startWrite);
    
    // Performance expectations (reasonable for the file size)
    EXPECT_LT(parseTime.count(), 1000);  // Should parse in less than 1 second
    EXPECT_LT(analyzeTime.count(), 100);  // Should analyze in less than 100ms
    EXPECT_LT(writeTime.count(), 50);     // Should write in less than 50ms
    
    std::cout << "Performance Results:" << std::endl;
    std::cout << "  Parse time: " << parseTime.count() << " ms" << std::endl;
    std::cout << "  Analyze time: " << analyzeTime.count() << " ms" << std::endl;
    std::cout << "  Write time: " << writeTime.count() << " ms" << std::endl;
    std::cout << "  Total time: " << (parseTime + analyzeTime + writeTime).count() << " ms" << std::endl;
}

TEST_F(IntegrationTest, MemoryUsage) {
    auto reader = DXFReaderFactory::createReader();
    
    // Read mesh data
    auto meshData = reader->readFile(designPitPath);
    
    // Verify reasonable memory usage
    size_t triangleCount = meshData->getTriangleCount();
    size_t expectedMemoryPerTriangle = sizeof(Triangle);  // Each triangle has 3 Point3D
    size_t minExpectedMemory = triangleCount * expectedMemoryPerTriangle;
    
    // This is a rough estimate - actual memory usage will be higher due to overhead
    EXPECT_GT(minExpectedMemory, 0);
    
    std::cout << "Memory estimates:" << std::endl;
    std::cout << "  Triangles: " << triangleCount << std::endl;
    std::cout << "  Bytes per triangle: " << expectedMemoryPerTriangle << std::endl;
    std::cout << "  Estimated minimum memory: " << minExpectedMemory << " bytes" << std::endl;
}

TEST_F(IntegrationTest, RepeatedProcessing) {
    // Test that the same file can be processed multiple times with consistent results
    auto reader = DXFReaderFactory::createReader();
    
    auto meshData1 = reader->readFile(designPitPath);
    auto meshData2 = reader->readFile(designPitPath);
    
    EXPECT_EQ(meshData1->getTriangleCount(), meshData2->getTriangleCount());
    EXPECT_DOUBLE_EQ(meshData1->getTotalSurfaceArea(), meshData2->getTotalSurfaceArea());
    
    auto bbox1 = meshData1->getBoundingBox();
    auto bbox2 = meshData2->getBoundingBox();
    
    EXPECT_TRUE(bbox1.min == bbox2.min);
    EXPECT_TRUE(bbox1.max == bbox2.max);
}

TEST_F(IntegrationTest, EndToEndConsistency) {
    // Complete end-to-end test with all components
    auto reader = DXFReaderFactory::createReader();
    auto summarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Detailed);
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    
    writer->setIncludeTimestamp(false);
    writer->setPrettyPrint(true);
    
    // Process the file
    auto meshData = reader->readFile(designPitPath);
    auto summary = summarizer->summarize(*meshData);
    std::string outputPath = writer->writeToFile(summary, "end_to_end");
    
    // Verify all stages completed successfully
    EXPECT_EQ(meshData->getTriangleCount(), 2929);
    EXPECT_EQ(summary.triangleCount, 2929);
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    
    // Verify the written file is substantial
    EXPECT_GT(std::filesystem::file_size(outputPath), 1000);
    
    std::cout << "End-to-end test completed successfully" << std::endl;
    std::cout << "  Input: " << designPitPath << std::endl;
    std::cout << "  Output: " << outputPath << std::endl;
    std::cout << "  Triangles processed: " << meshData->getTriangleCount() << std::endl;
    std::cout << "  Surface area: " << summary.totalSurfaceArea << std::endl;
}