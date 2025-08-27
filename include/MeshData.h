#pragma once

#include <vector>
#include <array>
#include <limits>
#include <cmath>

namespace DXFProcessor {

    struct Point3D {
        double x, y, z;
        
        Point3D() : x(0.0), y(0.0), z(0.0) {}
        Point3D(double x, double y, double z) : x(x), y(y), z(z) {}
        
        Point3D operator+(const Point3D& other) const {
            return Point3D(x + other.x, y + other.y, z + other.z);
        }
        
        Point3D operator-(const Point3D& other) const {
            return Point3D(x - other.x, y - other.y, z - other.z);
        }
        
        Point3D operator*(double scalar) const {
            return Point3D(x * scalar, y * scalar, z * scalar);
        }
        
        bool operator==(const Point3D& other) const {
            const double epsilon = 1e-10;
            return std::abs(x - other.x) < epsilon &&
                   std::abs(y - other.y) < epsilon &&
                   std::abs(z - other.z) < epsilon;
        }
        
        double dot(const Point3D& other) const {
            return x * other.x + y * other.y + z * other.z;
        }
        
        Point3D cross(const Point3D& other) const {
            return Point3D(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            );
        }
        
        double magnitude() const {
            return sqrt(x * x + y * y + z * z);
        }
    };

    struct Triangle {
        std::array<Point3D, 3> vertices;
        
        Triangle() = default;
        Triangle(const Point3D& v1, const Point3D& v2, const Point3D& v3) 
            : vertices{v1, v2, v3} {}
        
        Point3D normal() const {
            Point3D edge1 = vertices[1] - vertices[0];
            Point3D edge2 = vertices[2] - vertices[0];
            return edge1.cross(edge2);
        }
        
        double area() const {
            return normal().magnitude() * 0.5;
        }
        
        Point3D center() const {
            return (vertices[0] + vertices[1] + vertices[2]) * (1.0 / 3.0);
        }
    };

    struct BoundingBox {
        Point3D min, max;
        
        BoundingBox() 
            : min(std::numeric_limits<double>::max(), 
                  std::numeric_limits<double>::max(), 
                  std::numeric_limits<double>::max())
            , max(std::numeric_limits<double>::lowest(), 
                  std::numeric_limits<double>::lowest(), 
                  std::numeric_limits<double>::lowest()) {}
        
        void expand(const Point3D& point) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            min.z = std::min(min.z, point.z);
            
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
            max.z = std::max(max.z, point.z);
        }
        
        Point3D size() const {
            return max - min;
        }
        
        Point3D center() const {
            return (min + max) * 0.5;
        }
        
        double volume() const {
            Point3D s = size();
            return s.x * s.y * s.z;
        }
        
        bool isEmpty() const {
            return min.x > max.x || min.y > max.y || min.z > max.z;
        }
    };

    class MeshData {
    public:
        std::vector<Triangle> triangles;
        
        MeshData() = default;
        
        void addTriangle(const Triangle& triangle) {
            triangles.push_back(triangle);
        }
        
        void addTriangle(const Point3D& v1, const Point3D& v2, const Point3D& v3) {
            triangles.emplace_back(v1, v2, v3);
        }
        
        void clear() {
            triangles.clear();
        }
        
        size_t getTriangleCount() const {
            return triangles.size();
        }
        
        bool isEmpty() const {
            return triangles.empty();
        }
        
        BoundingBox getBoundingBox() const {
            BoundingBox bbox;
            for (const auto& triangle : triangles) {
                for (const auto& vertex : triangle.vertices) {
                    bbox.expand(vertex);
                }
            }
            return bbox;
        }
        
        double getTotalSurfaceArea() const {
            double totalArea = 0.0;
            for (const auto& triangle : triangles) {
                totalArea += triangle.area();
            }
            return totalArea;
        }
        
        void reserve(size_t capacity) {
            triangles.reserve(capacity);
        }
    };

} // namespace DXFProcessor