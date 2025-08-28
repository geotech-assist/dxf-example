/**
 * @file test_mesh_summarizer.cpp
 * @brief Unit tests for MeshSummarizer functionality
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MeshSummarizer.h"
#include "MeshData.h"

using namespace DXFProcessor;

class MeshSummarizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        meshData = std::make_unique<MeshData>();
        
        // Create a simple test mesh with two triangles
        // Triangle 1: Unit triangle at origin
        triangle1 = Triangle(
            Point3D(0.0, 0.0, 0.0),
            Point3D(1.0, 0.0, 0.0),
            Point3D(0.0, 1.0, 0.0)
        );
        
        // Triangle 2: Unit triangle offset
        triangle2 = Triangle(
            Point3D(2.0, 0.0, 0.0),
            Point3D(3.0, 0.0, 0.0),
            Point3D(2.0, 1.0, 0.0)
        );
        
        meshData->addTriangle(triangle1);
        meshData->addTriangle(triangle2);
        
        basicSummarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Basic);
        detailedSummarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Detailed);
    }
    
    std::unique_ptr<MeshData> meshData;
    Triangle triangle1, triangle2;
    std::unique_ptr<MeshSummarizer> basicSummarizer;
    std::unique_ptr<MeshSummarizer> detailedSummarizer;
};

TEST_F(MeshSummarizerTest, FactoryCreateBasicSummarizer) {
    auto summarizer = MeshSummarizerFactory::create();
    EXPECT_NE(summarizer, nullptr);
    
    auto summary = summarizer->summarize(*meshData);
    EXPECT_EQ(summary.triangleCount, 2);
}

TEST_F(MeshSummarizerTest, FactoryCreateDetailedSummarizer) {
    auto summarizer = MeshSummarizerFactory::create(MeshSummarizerFactory::SummarizerType::Detailed);
    EXPECT_NE(summarizer, nullptr);
    
    auto summary = summarizer->summarize(*meshData);
    EXPECT_EQ(summary.triangleCount, 2);
    
    // Detailed summarizer should have additional custom fields
    EXPECT_FALSE(summary.customFields.empty());
}

TEST_F(MeshSummarizerTest, FactoryCreateFromString) {
    auto basicFromString = MeshSummarizerFactory::create("basic");
    auto detailedFromString = MeshSummarizerFactory::create("detailed");
    auto defaultFromEmpty = MeshSummarizerFactory::create("");
    
    EXPECT_NE(basicFromString, nullptr);
    EXPECT_NE(detailedFromString, nullptr);
    EXPECT_NE(defaultFromEmpty, nullptr);
}

TEST_F(MeshSummarizerTest, BasicSummaryTriangleCount) {
    auto summary = basicSummarizer->summarize(*meshData);
    
    EXPECT_EQ(summary.triangleCount, 2);
}

TEST_F(MeshSummarizerTest, BasicSummaryTotalSurfaceArea) {
    auto summary = basicSummarizer->summarize(*meshData);
    
    // Each triangle has area 0.5, so total should be 1.0
    EXPECT_DOUBLE_EQ(summary.totalSurfaceArea, 1.0);
}

TEST_F(MeshSummarizerTest, BasicSummaryBoundingBox) {
    auto summary = basicSummarizer->summarize(*meshData);
    
    EXPECT_DOUBLE_EQ(summary.boundingBox.min.x, 0.0);
    EXPECT_DOUBLE_EQ(summary.boundingBox.min.y, 0.0);
    EXPECT_DOUBLE_EQ(summary.boundingBox.min.z, 0.0);
    
    EXPECT_DOUBLE_EQ(summary.boundingBox.max.x, 3.0);
    EXPECT_DOUBLE_EQ(summary.boundingBox.max.y, 1.0);
    EXPECT_DOUBLE_EQ(summary.boundingBox.max.z, 0.0);
}

TEST_F(MeshSummarizerTest, BasicSummaryCentroid) {
    auto summary = basicSummarizer->summarize(*meshData);
    
    // Centroid should be area-weighted average of triangle centers
    // Triangle 1 center: (1/3, 1/3, 0), area: 0.5
    // Triangle 2 center: (7/3, 1/3, 0), area: 0.5
    // Weighted average: ((1/3 + 7/3)/2, (1/3 + 1/3)/2, 0) = (4/3, 1/3, 0)
    
    EXPECT_NEAR(summary.centroid.x, 4.0/3.0, 0.001);
    EXPECT_NEAR(summary.centroid.y, 1.0/3.0, 0.001);
    EXPECT_DOUBLE_EQ(summary.centroid.z, 0.0);
}

TEST_F(MeshSummarizerTest, BasicSummaryCustomFields) {
    auto summary = basicSummarizer->summarize(*meshData);
    
    // Basic summarizer should have some custom fields
    EXPECT_FALSE(summary.customFields.empty());
    
    // Check for expected fields
    EXPECT_NE(summary.getCustomField("average_triangle_area"), "");
    EXPECT_NE(summary.getCustomField("bounding_box_volume"), "");
    EXPECT_NE(summary.getCustomField("mesh_density"), "");
}

TEST_F(MeshSummarizerTest, DetailedSummaryHasMoreFields) {
    auto basicSummary = basicSummarizer->summarize(*meshData);
    auto detailedSummary = detailedSummarizer->summarize(*meshData);
    
    // Detailed summarizer should have more custom fields than basic
    EXPECT_GT(detailedSummary.customFields.size(), basicSummary.customFields.size());
    
    // Check for detailed-specific fields
    EXPECT_NE(detailedSummary.getCustomField("volume_estimate"), "");
    EXPECT_NE(detailedSummary.getCustomField("min_triangle_area"), "");
    EXPECT_NE(detailedSummary.getCustomField("max_triangle_area"), "");
}

TEST_F(MeshSummarizerTest, EmptyMeshSummary) {
    auto emptyMesh = std::make_unique<MeshData>();
    auto summary = basicSummarizer->summarize(*emptyMesh);
    
    EXPECT_EQ(summary.triangleCount, 0);
    EXPECT_DOUBLE_EQ(summary.totalSurfaceArea, 0.0);
    EXPECT_TRUE(summary.boundingBox.isEmpty());
}

TEST_F(MeshSummarizerTest, SingleTriangleMesh) {
    auto singleTriangleMesh = std::make_unique<MeshData>();
    singleTriangleMesh->addTriangle(triangle1);
    
    auto summary = basicSummarizer->summarize(*singleTriangleMesh);
    
    EXPECT_EQ(summary.triangleCount, 1);
    EXPECT_DOUBLE_EQ(summary.totalSurfaceArea, 0.5);
}

TEST_F(MeshSummarizerTest, CustomFieldAddition) {
    auto summary = basicSummarizer->summarize(*meshData);
    
    summary.addCustomField("test_field", "test_value");
    EXPECT_EQ(summary.getCustomField("test_field"), "test_value");
    
    summary.addCustomField("numeric_field", "42.0");
    EXPECT_EQ(summary.getCustomField("numeric_field"), "42.0");
}

TEST_F(MeshSummarizerTest, CustomFieldRetrieval) {
    auto summary = basicSummarizer->summarize(*meshData);
    
    // Existing field
    std::string avgArea = summary.getCustomField("average_triangle_area");
    EXPECT_NE(avgArea, "");
    
    // Non-existing field should return empty string
    std::string nonExistent = summary.getCustomField("does_not_exist");
    EXPECT_EQ(nonExistent, "");
}

TEST_F(MeshSummarizerTest, DetailedVolumeEstimate) {
    // Create a tetrahedron for volume testing
    auto tetrahedronMesh = std::make_unique<MeshData>();
    
    // Define vertices for a unit tetrahedron
    Point3D v0(0.0, 0.0, 0.0);
    Point3D v1(1.0, 0.0, 0.0);
    Point3D v2(0.0, 1.0, 0.0);
    Point3D v3(0.0, 0.0, 1.0);
    
    // Four faces of the tetrahedron (oriented consistently outward)
    tetrahedronMesh->addTriangle(v0, v2, v1);  // Bottom face (CCW from below)
    tetrahedronMesh->addTriangle(v0, v1, v3);  // Front face
    tetrahedronMesh->addTriangle(v1, v2, v3);  // Right face
    tetrahedronMesh->addTriangle(v2, v0, v3);  // Left face
    
    auto summary = detailedSummarizer->summarize(*tetrahedronMesh);
    
    // Volume estimate should be non-zero
    std::string volumeStr = summary.getCustomField("volume_estimate");
    EXPECT_NE(volumeStr, "");
    
    double volume = std::stod(volumeStr);
    EXPECT_GT(volume, 0.0);
    
    // Expected volume of this tetrahedron is 1/6
    EXPECT_NEAR(volume, 1.0/6.0, 0.01);
}

TEST_F(MeshSummarizerTest, DetailedTriangleAreaStatistics) {
    // Create mesh with triangles of different sizes
    auto mixedMesh = std::make_unique<MeshData>();
    
    // Small triangle (area = 0.5)
    mixedMesh->addTriangle(
        Point3D(0.0, 0.0, 0.0),
        Point3D(1.0, 0.0, 0.0),
        Point3D(0.0, 1.0, 0.0)
    );
    
    // Large triangle (area = 2.0)
    mixedMesh->addTriangle(
        Point3D(0.0, 0.0, 0.0),
        Point3D(2.0, 0.0, 0.0),
        Point3D(0.0, 2.0, 0.0)
    );
    
    auto summary = detailedSummarizer->summarize(*mixedMesh);
    
    double minArea = std::stod(summary.getCustomField("min_triangle_area"));
    double maxArea = std::stod(summary.getCustomField("max_triangle_area"));
    
    EXPECT_NEAR(minArea, 0.5, 0.001);
    EXPECT_NEAR(maxArea, 2.0, 0.001);
}