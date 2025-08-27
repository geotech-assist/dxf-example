#include "MeshSummarizer.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace DXFProcessor {

    MeshSummary MeshSummarizer::summarize(const MeshData& meshData) {
        MeshSummary summary;
        
        calculateBasicStats(meshData, summary);
        calculateAdvancedStats(meshData, summary);
        addCustomCalculations(meshData, summary);
        
        return summary;
    }

    void MeshSummarizer::calculateBasicStats(const MeshData& meshData, MeshSummary& summary) {
        summary.triangleCount = meshData.getTriangleCount();
        summary.boundingBox = meshData.getBoundingBox();
        summary.totalSurfaceArea = meshData.getTotalSurfaceArea();
        summary.centroid = calculateCentroid(meshData);
    }

    void MeshSummarizer::calculateAdvancedStats(const MeshData& meshData, MeshSummary& summary) {
        if (meshData.isEmpty()) {
            return;
        }
        
        summary.addCustomField("mesh_density", 
            std::to_string(summary.triangleCount / summary.boundingBox.volume()));
        
        summary.addCustomField("average_triangle_area", 
            std::to_string(summary.totalSurfaceArea / summary.triangleCount));
        
        Point3D size = summary.boundingBox.size();
        summary.addCustomField("bounding_box_volume", std::to_string(summary.boundingBox.volume()));
        summary.addCustomField("width", std::to_string(size.x));
        summary.addCustomField("height", std::to_string(size.y));
        summary.addCustomField("depth", std::to_string(size.z));
    }

    void MeshSummarizer::addCustomCalculations(const MeshData& meshData, MeshSummary& summary) {
        // Base implementation - can be overridden by derived classes
    }

    Point3D MeshSummarizer::calculateCentroid(const MeshData& meshData) {
        if (meshData.isEmpty()) {
            return Point3D(0, 0, 0);
        }
        
        Point3D centroid(0, 0, 0);
        double totalArea = 0.0;
        
        for (const auto& triangle : meshData.triangles) {
            double area = triangle.area();
            Point3D triangleCenter = triangle.center();
            
            centroid = centroid + (triangleCenter * area);
            totalArea += area;
        }
        
        if (totalArea > 0.0) {
            centroid = centroid * (1.0 / totalArea);
        }
        
        return centroid;
    }

    // DetailedMeshSummarizer implementation

    void DetailedMeshSummarizer::addCustomCalculations(const MeshData& meshData, MeshSummary& summary) {
        if (meshData.isEmpty()) {
            return;
        }
        
        summary.addCustomField("volume_estimate", std::to_string(calculateVolume(meshData)));
        
        auto [minArea, maxArea] = getTriangleAreaRange(meshData);
        summary.addCustomField("min_triangle_area", std::to_string(minArea));
        summary.addCustomField("max_triangle_area", std::to_string(maxArea));
        summary.addCustomField("triangle_area_variance", 
            std::to_string(maxArea - minArea));
        
        summary.addCustomField("compactness_ratio", 
            std::to_string(summary.totalSurfaceArea / summary.boundingBox.volume()));
        
        double avgArea = calculateAverageTriangleArea(meshData);
        summary.addCustomField("average_triangle_area_detailed", std::to_string(avgArea));
        
        size_t smallTriangles = 0, largeTriangles = 0;
        for (const auto& triangle : meshData.triangles) {
            double area = triangle.area();
            if (area < avgArea * 0.5) smallTriangles++;
            if (area > avgArea * 2.0) largeTriangles++;
        }
        
        summary.addCustomField("small_triangles_count", std::to_string(smallTriangles));
        summary.addCustomField("large_triangles_count", std::to_string(largeTriangles));
        summary.addCustomField("small_triangles_percentage", 
            std::to_string((double)smallTriangles / summary.triangleCount * 100.0));
        summary.addCustomField("large_triangles_percentage", 
            std::to_string((double)largeTriangles / summary.triangleCount * 100.0));
    }

    double DetailedMeshSummarizer::calculateVolume(const MeshData& meshData) {
        if (meshData.isEmpty()) {
            return 0.0;
        }
        
        double volume = 0.0;
        Point3D origin(0, 0, 0);
        
        for (const auto& triangle : meshData.triangles) {
            Point3D v0 = triangle.vertices[0];
            Point3D v1 = triangle.vertices[1];
            Point3D v2 = triangle.vertices[2];
            
            volume += (v0.dot(v1.cross(v2))) / 6.0;
        }
        
        return std::abs(volume);
    }

    double DetailedMeshSummarizer::calculateAverageTriangleArea(const MeshData& meshData) {
        if (meshData.isEmpty()) {
            return 0.0;
        }
        
        return meshData.getTotalSurfaceArea() / meshData.getTriangleCount();
    }

    std::pair<double, double> DetailedMeshSummarizer::getTriangleAreaRange(const MeshData& meshData) {
        if (meshData.isEmpty()) {
            return {0.0, 0.0};
        }
        
        double minArea = std::numeric_limits<double>::max();
        double maxArea = std::numeric_limits<double>::lowest();
        
        for (const auto& triangle : meshData.triangles) {
            double area = triangle.area();
            minArea = std::min(minArea, area);
            maxArea = std::max(maxArea, area);
        }
        
        return {minArea, maxArea};
    }

} // namespace DXFProcessor