#pragma once

#include <vector>
#include <array>
#include <limits>
#include <cmath>

namespace DXFProcessor {

    /**
     * @brief 3D point with double precision coordinates and vector operations
     * 
     * Represents a point in 3D space with comprehensive mathematical operations
     * commonly needed for mesh processing and geometric calculations.
     */
    struct Point3D {
        double x, y, z;  ///< Cartesian coordinates
        
        Point3D() : x(0.0), y(0.0), z(0.0) {}  ///< Default constructor (origin)
        Point3D(double x, double y, double z) : x(x), y(y), z(z) {}  ///< Construct with coordinates
        
        /**
         * @brief Vector addition
         * @param other Point to add
         * @return Sum of the two points
         */
        Point3D operator+(const Point3D& other) const {
            return Point3D(x + other.x, y + other.y, z + other.z);
        }
        
        /**
         * @brief Vector subtraction
         * @param other Point to subtract
         * @return Difference between the two points
         */
        Point3D operator-(const Point3D& other) const {
            return Point3D(x - other.x, y - other.y, z - other.z);
        }
        
        /**
         * @brief Scalar multiplication
         * @param scalar Value to multiply by
         * @return Scaled point
         */
        Point3D operator*(double scalar) const {
            return Point3D(x * scalar, y * scalar, z * scalar);
        }
        
        /**
         * @brief Equality comparison with epsilon tolerance
         * @param other Point to compare against
         * @return true if points are equal within tolerance (1e-9)
         */
        bool operator==(const Point3D& other) const {
            const double epsilon = 1e-9;
            return std::abs(x - other.x) < epsilon &&
                   std::abs(y - other.y) < epsilon &&
                   std::abs(z - other.z) < epsilon;
        }
        
        /**
         * @brief Dot product of two vectors
         * @param other Other vector
         * @return Scalar dot product
         */
        double dot(const Point3D& other) const {
            return x * other.x + y * other.y + z * other.z;
        }
        
        /**
         * @brief Cross product of two vectors
         * @param other Other vector
         * @return Vector perpendicular to both input vectors
         */
        Point3D cross(const Point3D& other) const {
            return Point3D(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            );
        }
        
        /**
         * @brief Calculate vector magnitude (length)
         * @return Length of the vector from origin to this point
         */
        double magnitude() const {
            return sqrt(x * x + y * y + z * z);
        }
    };

    /**
     * @brief Triangle primitive with vertex data and geometric operations
     * 
     * Represents a triangular face with three vertices and provides common
     * geometric calculations like area, normal vector, and centroid.
     */
    struct Triangle {
        std::array<Point3D, 3> vertices;  ///< The three vertices of the triangle
        
        Triangle() = default;  ///< Default constructor
        /**
         * @brief Construct triangle from three vertices
         * @param v1 First vertex
         * @param v2 Second vertex  
         * @param v3 Third vertex
         */
        Triangle(const Point3D& v1, const Point3D& v2, const Point3D& v3) 
            : vertices{v1, v2, v3} {}
        
        /**
         * @brief Calculate triangle normal vector (not normalized)
         * @return Normal vector using cross product of two edges
         */
        Point3D normal() const {
            Point3D edge1 = vertices[1] - vertices[0];
            Point3D edge2 = vertices[2] - vertices[0];
            return edge1.cross(edge2);
        }
        
        /**
         * @brief Calculate triangle area
         * @return Surface area of the triangle
         */
        double area() const {
            return normal().magnitude() * 0.5;
        }
        
        /**
         * @brief Calculate triangle centroid (geometric center)
         * @return Center point of the triangle
         */
        Point3D center() const {
            return (vertices[0] + vertices[1] + vertices[2]) * (1.0 / 3.0);
        }
    };

    /**
     * @brief Axis-aligned bounding box for 3D geometry
     * 
     * Tracks minimum and maximum extents in all three dimensions,
     * commonly used for spatial queries and mesh analysis.
     */
    struct BoundingBox {
        Point3D min, max;
        
        /**
         * @brief Default constructor creates empty bounding box
         * 
         * Initializes min to maximum values and max to minimum values
         * so that the first point added will properly initialize the bounds.
         */
        BoundingBox() 
            : min(std::numeric_limits<double>::max(), 
                  std::numeric_limits<double>::max(), 
                  std::numeric_limits<double>::max())
            , max(std::numeric_limits<double>::lowest(), 
                  std::numeric_limits<double>::lowest(), 
                  std::numeric_limits<double>::lowest()) {}
        
        /**
         * @brief Expand bounding box to include a new point
         * @param point Point to include in the bounding box
         */
        void expand(const Point3D& point) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            min.z = std::min(min.z, point.z);
            
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
            max.z = std::max(max.z, point.z);
        }
        
        /**
         * @brief Get dimensions of the bounding box
         * @return Size vector (width, height, depth)
         */
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