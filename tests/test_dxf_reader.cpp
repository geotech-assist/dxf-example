/**
 * @file test_dxf_reader.cpp
 * @brief Unit tests for DXFReader functionality
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "DXFReader.h"
#include <filesystem>
#include <fstream>

using namespace DXFProcessor;

class DXFReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        reader = DXFReaderFactory::createReader();
        testDataDir = TEST_DATA_DIR;
        
        // Set up progress tracking for tests
        progressValues.clear();
        reader->setProgressCallback([this](double progress) {
            progressValues.push_back(progress);
        });
    }
    
    void TearDown() override {
        reader.reset();
    }
    
    std::unique_ptr<DXFReader> reader;
    std::string testDataDir;
    std::vector<double> progressValues;
};

TEST_F(DXFReaderTest, FactoryCreateReader) {
    auto reader1 = DXFReaderFactory::createReader();
    auto reader2 = DXFReaderFactory::createReader("standard");
    
    EXPECT_NE(reader1, nullptr);
    EXPECT_NE(reader2, nullptr);
}

TEST_F(DXFReaderTest, FactoryInvalidReaderType) {
    EXPECT_THROW(DXFReaderFactory::createReader("invalid_type"), DXFReaderException);
}

TEST_F(DXFReaderTest, ReadNonExistentFile) {
    std::string nonExistentFile = testDataDir + "/does_not_exist.dxf";
    
    EXPECT_THROW(reader->readFile(nonExistentFile), DXFReaderException);
}

TEST_F(DXFReaderTest, ReadDirectoryInsteadOfFile) {
    // Try to read the test data directory as if it were a file
    EXPECT_THROW(reader->readFile(testDataDir), DXFReaderException);
}

TEST_F(DXFReaderTest, ReadEmptyDXFFile) {
    std::string emptyFile = testDataDir + "/empty.dxf";
    
    EXPECT_THROW(reader->readFile(emptyFile), DXFReaderException);
}

TEST_F(DXFReaderTest, ReadMalformedDXFFile) {
    std::string malformedFile = testDataDir + "/malformed.dxf";
    
    EXPECT_THROW(reader->readFile(malformedFile), DXFReaderException);
}

TEST_F(DXFReaderTest, ReadSingleTriangleDXF) {
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    
    auto meshData = reader->readFile(singleTriangleFile);
    
    EXPECT_NE(meshData, nullptr);
    EXPECT_EQ(meshData->getTriangleCount(), 1);
    EXPECT_EQ(reader->getLastEntityCount(), 1);
    
    // Check that progress was reported
    EXPECT_FALSE(progressValues.empty());
    EXPECT_DOUBLE_EQ(progressValues.back(), 1.0);  // Should end with 100%
}

TEST_F(DXFReaderTest, ReadTwoTrianglesDXF) {
    std::string twoTrianglesFile = testDataDir + "/two_triangles.dxf";
    
    auto meshData = reader->readFile(twoTrianglesFile);
    
    EXPECT_NE(meshData, nullptr);
    EXPECT_EQ(meshData->getTriangleCount(), 2);
    EXPECT_EQ(reader->getLastEntityCount(), 2);
}

TEST_F(DXFReaderTest, VerifyTriangleGeometry) {
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    
    auto meshData = reader->readFile(singleTriangleFile);
    
    ASSERT_EQ(meshData->getTriangleCount(), 1);
    
    const Triangle& triangle = meshData->triangles[0];
    
    // Verify vertices match what we put in the test file
    EXPECT_DOUBLE_EQ(triangle.vertices[0].x, 0.0);
    EXPECT_DOUBLE_EQ(triangle.vertices[0].y, 0.0);
    EXPECT_DOUBLE_EQ(triangle.vertices[0].z, 0.0);
    
    EXPECT_DOUBLE_EQ(triangle.vertices[1].x, 10.0);
    EXPECT_DOUBLE_EQ(triangle.vertices[1].y, 0.0);
    EXPECT_DOUBLE_EQ(triangle.vertices[1].z, 0.0);
    
    EXPECT_DOUBLE_EQ(triangle.vertices[2].x, 5.0);
    EXPECT_NEAR(triangle.vertices[2].y, 8.660254, 0.001);
    EXPECT_DOUBLE_EQ(triangle.vertices[2].z, 0.0);
}

TEST_F(DXFReaderTest, CalculateTriangleArea) {
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    
    auto meshData = reader->readFile(singleTriangleFile);
    
    ASSERT_EQ(meshData->getTriangleCount(), 1);
    
    double totalArea = meshData->getTotalSurfaceArea();
    
    // Expected area for equilateral triangle with side length 10
    double expectedArea = (std::sqrt(3.0) / 4.0) * 10.0 * 10.0;
    EXPECT_NEAR(totalArea, expectedArea, 0.01);
}

TEST_F(DXFReaderTest, BoundingBoxCalculation) {
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    
    auto meshData = reader->readFile(singleTriangleFile);
    
    BoundingBox bbox = meshData->getBoundingBox();
    
    EXPECT_DOUBLE_EQ(bbox.min.x, 0.0);
    EXPECT_DOUBLE_EQ(bbox.min.y, 0.0);
    EXPECT_DOUBLE_EQ(bbox.min.z, 0.0);
    
    EXPECT_DOUBLE_EQ(bbox.max.x, 10.0);
    EXPECT_NEAR(bbox.max.y, 8.660254, 0.001);
    EXPECT_DOUBLE_EQ(bbox.max.z, 0.0);
}

TEST_F(DXFReaderTest, ProgressCallbackCalled) {
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    
    reader->readFile(singleTriangleFile);
    
    // Progress should have been called at least once
    EXPECT_FALSE(progressValues.empty());
    
    // All progress values should be between 0 and 1
    for (double progress : progressValues) {
        EXPECT_GE(progress, 0.0);
        EXPECT_LE(progress, 1.0);
    }
    
    // Last progress value should be 1.0 (100%)
    EXPECT_DOUBLE_EQ(progressValues.back(), 1.0);
}

TEST_F(DXFReaderTest, ProgressCallbackNotSet) {
    // Create a reader without progress callback
    auto readerNoCallback = DXFReaderFactory::createReader();
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    
    // Should not crash even without progress callback
    EXPECT_NO_THROW(readerNoCallback->readFile(singleTriangleFile));
}

TEST_F(DXFReaderTest, MultipleFileReads) {
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    std::string twoTrianglesFile = testDataDir + "/two_triangles.dxf";
    
    // Read first file
    auto mesh1 = reader->readFile(singleTriangleFile);
    EXPECT_EQ(mesh1->getTriangleCount(), 1);
    EXPECT_EQ(reader->getLastEntityCount(), 1);
    
    // Read second file with same reader
    auto mesh2 = reader->readFile(twoTrianglesFile);
    EXPECT_EQ(mesh2->getTriangleCount(), 2);
    EXPECT_EQ(reader->getLastEntityCount(), 2);
}

class MockProgressCallback {
public:
    MOCK_METHOD(void, onProgress, (double progress), ());
};

TEST_F(DXFReaderTest, ProgressCallbackWithMock) {
    MockProgressCallback mockCallback;
    std::string singleTriangleFile = testDataDir + "/single_triangle.dxf";
    
    // Expect final progress to be 1.0 (this should always be called)
    EXPECT_CALL(mockCallback, onProgress(1.0))
        .Times(::testing::AtLeast(1));
    
    // Allow any other progress values
    EXPECT_CALL(mockCallback, onProgress(::testing::Ne(1.0)))
        .Times(::testing::AnyNumber());
    
    reader->setProgressCallback([&mockCallback](double progress) {
        mockCallback.onProgress(progress);
    });
    
    reader->readFile(singleTriangleFile);
}