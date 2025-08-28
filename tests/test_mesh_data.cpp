/**
 * @file test_mesh_data.cpp
 * @brief Unit tests for MeshData structures (Point3D, Triangle, BoundingBox, MeshData)
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MeshData.h"
#include <cmath>

using namespace DXFProcessor;
using ::testing::DoubleEq;
using ::testing::DoubleNear;

class Point3DTest : public ::testing::Test {
protected:
    void SetUp() override {
        origin = Point3D(0.0, 0.0, 0.0);
        unitX = Point3D(1.0, 0.0, 0.0);
        unitY = Point3D(0.0, 1.0, 0.0);
        unitZ = Point3D(0.0, 0.0, 1.0);
        point = Point3D(3.0, 4.0, 0.0);
    }
    
    Point3D origin, unitX, unitY, unitZ, point;
};

TEST_F(Point3DTest, DefaultConstructor) {
    Point3D p;
    EXPECT_DOUBLE_EQ(p.x, 0.0);
    EXPECT_DOUBLE_EQ(p.y, 0.0);
    EXPECT_DOUBLE_EQ(p.z, 0.0);
}

TEST_F(Point3DTest, ParameterizedConstructor) {
    Point3D p(1.5, 2.5, 3.5);
    EXPECT_DOUBLE_EQ(p.x, 1.5);
    EXPECT_DOUBLE_EQ(p.y, 2.5);
    EXPECT_DOUBLE_EQ(p.z, 3.5);
}

TEST_F(Point3DTest, Addition) {
    Point3D result = unitX + unitY;
    EXPECT_DOUBLE_EQ(result.x, 1.0);
    EXPECT_DOUBLE_EQ(result.y, 1.0);
    EXPECT_DOUBLE_EQ(result.z, 0.0);
}

TEST_F(Point3DTest, Subtraction) {
    Point3D result = unitX - unitY;
    EXPECT_DOUBLE_EQ(result.x, 1.0);
    EXPECT_DOUBLE_EQ(result.y, -1.0);
    EXPECT_DOUBLE_EQ(result.z, 0.0);
}

TEST_F(Point3DTest, ScalarMultiplication) {
    Point3D result = point * 2.0;
    EXPECT_DOUBLE_EQ(result.x, 6.0);
    EXPECT_DOUBLE_EQ(result.y, 8.0);
    EXPECT_DOUBLE_EQ(result.z, 0.0);
}

TEST_F(Point3DTest, EqualityComparison) {
    Point3D p1(1.0, 2.0, 3.0);
    Point3D p2(1.0, 2.0, 3.0);
    Point3D p3(1.1, 2.0, 3.0);
    
    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);
}

TEST_F(Point3DTest, EqualityWithEpsilon) {
    Point3D p1(1.0, 2.0, 3.0);
    Point3D p2(1.0000000001, 2.0, 3.0);  // Very small difference
    
    EXPECT_TRUE(p1 == p2);  // Should be equal within epsilon tolerance
}

TEST_F(Point3DTest, DotProduct) {
    EXPECT_DOUBLE_EQ(unitX.dot(unitX), 1.0);
    EXPECT_DOUBLE_EQ(unitX.dot(unitY), 0.0);
    EXPECT_DOUBLE_EQ(point.dot(unitX), 3.0);
}

TEST_F(Point3DTest, CrossProduct) {
    Point3D result = unitX.cross(unitY);
    EXPECT_DOUBLE_EQ(result.x, 0.0);
    EXPECT_DOUBLE_EQ(result.y, 0.0);
    EXPECT_DOUBLE_EQ(result.z, 1.0);
    
    // Test anti-commutativity
    Point3D result2 = unitY.cross(unitX);
    EXPECT_DOUBLE_EQ(result2.z, -1.0);
}

TEST_F(Point3DTest, Magnitude) {
    EXPECT_DOUBLE_EQ(unitX.magnitude(), 1.0);
    EXPECT_DOUBLE_EQ(point.magnitude(), 5.0);  // 3-4-5 triangle
    EXPECT_DOUBLE_EQ(origin.magnitude(), 0.0);
}

class TriangleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a right triangle in XY plane
        v1 = Point3D(0.0, 0.0, 0.0);
        v2 = Point3D(3.0, 0.0, 0.0);
        v3 = Point3D(0.0, 4.0, 0.0);
        triangle = Triangle(v1, v2, v3);
    }
    
    Point3D v1, v2, v3;
    Triangle triangle;
};

TEST_F(TriangleTest, DefaultConstructor) {
    Triangle t;
    // Should construct successfully (vertices initialized to zero)
    EXPECT_NO_THROW(t.area());
}

TEST_F(TriangleTest, ParameterizedConstructor) {
    EXPECT_DOUBLE_EQ(triangle.vertices[0].x, 0.0);
    EXPECT_DOUBLE_EQ(triangle.vertices[1].x, 3.0);
    EXPECT_DOUBLE_EQ(triangle.vertices[2].y, 4.0);
}

TEST_F(TriangleTest, NormalVector) {
    Point3D normal = triangle.normal();
    // For a triangle in XY plane, normal should point in +Z direction
    EXPECT_DOUBLE_EQ(normal.x, 0.0);
    EXPECT_DOUBLE_EQ(normal.y, 0.0);
    EXPECT_DOUBLE_EQ(normal.z, 12.0);  // |cross product| = 3*4 = 12
}

TEST_F(TriangleTest, Area) {
    double area = triangle.area();
    EXPECT_DOUBLE_EQ(area, 6.0);  // (1/2) * base * height = (1/2) * 3 * 4
}

TEST_F(TriangleTest, Center) {
    Point3D center = triangle.center();
    EXPECT_DOUBLE_EQ(center.x, 1.0);  // (0+3+0)/3
    EXPECT_DOUBLE_EQ(center.y, 4.0/3.0);  // (0+0+4)/3
    EXPECT_DOUBLE_EQ(center.z, 0.0);  // (0+0+0)/3
}

class BoundingBoxTest : public ::testing::Test {
protected:
    void SetUp() override {
        bbox = BoundingBox();
        point1 = Point3D(-1.0, -2.0, -3.0);
        point2 = Point3D(4.0, 5.0, 6.0);
        point3 = Point3D(0.0, 1.0, 2.0);
    }
    
    BoundingBox bbox;
    Point3D point1, point2, point3;
};

TEST_F(BoundingBoxTest, DefaultConstructor) {
    // Default constructor should create empty bounding box
    EXPECT_TRUE(bbox.isEmpty());
}

TEST_F(BoundingBoxTest, ExpandWithSinglePoint) {
    bbox.expand(point1);
    
    EXPECT_DOUBLE_EQ(bbox.min.x, -1.0);
    EXPECT_DOUBLE_EQ(bbox.min.y, -2.0);
    EXPECT_DOUBLE_EQ(bbox.min.z, -3.0);
    EXPECT_DOUBLE_EQ(bbox.max.x, -1.0);
    EXPECT_DOUBLE_EQ(bbox.max.y, -2.0);
    EXPECT_DOUBLE_EQ(bbox.max.z, -3.0);
    
    EXPECT_FALSE(bbox.isEmpty());
}

TEST_F(BoundingBoxTest, ExpandWithMultiplePoints) {
    bbox.expand(point1);
    bbox.expand(point2);
    bbox.expand(point3);
    
    EXPECT_DOUBLE_EQ(bbox.min.x, -1.0);
    EXPECT_DOUBLE_EQ(bbox.min.y, -2.0);
    EXPECT_DOUBLE_EQ(bbox.min.z, -3.0);
    EXPECT_DOUBLE_EQ(bbox.max.x, 4.0);
    EXPECT_DOUBLE_EQ(bbox.max.y, 5.0);
    EXPECT_DOUBLE_EQ(bbox.max.z, 6.0);
}

TEST_F(BoundingBoxTest, Size) {
    bbox.expand(point1);
    bbox.expand(point2);
    
    Point3D size = bbox.size();
    EXPECT_DOUBLE_EQ(size.x, 5.0);  // 4 - (-1)
    EXPECT_DOUBLE_EQ(size.y, 7.0);  // 5 - (-2)
    EXPECT_DOUBLE_EQ(size.z, 9.0);  // 6 - (-3)
}

TEST_F(BoundingBoxTest, Center) {
    bbox.expand(point1);
    bbox.expand(point2);
    
    Point3D center = bbox.center();
    EXPECT_DOUBLE_EQ(center.x, 1.5);  // (-1 + 4) / 2
    EXPECT_DOUBLE_EQ(center.y, 1.5);  // (-2 + 5) / 2
    EXPECT_DOUBLE_EQ(center.z, 1.5);  // (-3 + 6) / 2
}

TEST_F(BoundingBoxTest, Volume) {
    bbox.expand(point1);
    bbox.expand(point2);
    
    double volume = bbox.volume();
    EXPECT_DOUBLE_EQ(volume, 315.0);  // 5 * 7 * 9
}

class MeshDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        meshData = std::make_unique<MeshData>();
        
        // Create test triangles
        triangle1 = Triangle(
            Point3D(0.0, 0.0, 0.0),
            Point3D(1.0, 0.0, 0.0),
            Point3D(0.0, 1.0, 0.0)
        );
        
        triangle2 = Triangle(
            Point3D(1.0, 1.0, 0.0),
            Point3D(2.0, 1.0, 0.0),
            Point3D(1.0, 2.0, 0.0)
        );
    }
    
    std::unique_ptr<MeshData> meshData;
    Triangle triangle1, triangle2;
};

TEST_F(MeshDataTest, EmptyMesh) {
    EXPECT_TRUE(meshData->isEmpty());
    EXPECT_EQ(meshData->getTriangleCount(), 0);
    EXPECT_DOUBLE_EQ(meshData->getTotalSurfaceArea(), 0.0);
}

TEST_F(MeshDataTest, AddSingleTriangle) {
    meshData->addTriangle(triangle1);
    
    EXPECT_FALSE(meshData->isEmpty());
    EXPECT_EQ(meshData->getTriangleCount(), 1);
    EXPECT_DOUBLE_EQ(meshData->getTotalSurfaceArea(), 0.5);
}

TEST_F(MeshDataTest, AddMultipleTriangles) {
    meshData->addTriangle(triangle1);
    meshData->addTriangle(triangle2);
    
    EXPECT_EQ(meshData->getTriangleCount(), 2);
    EXPECT_DOUBLE_EQ(meshData->getTotalSurfaceArea(), 1.0);  // Two triangles of 0.5 each
}

TEST_F(MeshDataTest, AddTriangleFromPoints) {
    meshData->addTriangle(
        Point3D(0.0, 0.0, 0.0),
        Point3D(2.0, 0.0, 0.0),
        Point3D(0.0, 2.0, 0.0)
    );
    
    EXPECT_EQ(meshData->getTriangleCount(), 1);
    EXPECT_DOUBLE_EQ(meshData->getTotalSurfaceArea(), 2.0);  // Area = 0.5 * 2 * 2
}

TEST_F(MeshDataTest, BoundingBox) {
    meshData->addTriangle(triangle1);
    meshData->addTriangle(triangle2);
    
    BoundingBox bbox = meshData->getBoundingBox();
    
    EXPECT_DOUBLE_EQ(bbox.min.x, 0.0);
    EXPECT_DOUBLE_EQ(bbox.min.y, 0.0);
    EXPECT_DOUBLE_EQ(bbox.min.z, 0.0);
    EXPECT_DOUBLE_EQ(bbox.max.x, 2.0);
    EXPECT_DOUBLE_EQ(bbox.max.y, 2.0);
    EXPECT_DOUBLE_EQ(bbox.max.z, 0.0);
}

TEST_F(MeshDataTest, Clear) {
    meshData->addTriangle(triangle1);
    meshData->addTriangle(triangle2);
    
    EXPECT_EQ(meshData->getTriangleCount(), 2);
    
    meshData->clear();
    
    EXPECT_TRUE(meshData->isEmpty());
    EXPECT_EQ(meshData->getTriangleCount(), 0);
}

TEST_F(MeshDataTest, Reserve) {
    meshData->reserve(1000);
    // Should not crash and should be able to add triangles
    meshData->addTriangle(triangle1);
    EXPECT_EQ(meshData->getTriangleCount(), 1);
}