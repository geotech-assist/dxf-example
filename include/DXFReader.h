#pragma once

#include "MeshData.h"
#include <string>
#include <memory>
#include <stdexcept>
#include <functional>

namespace DXFProcessor {

    /**
     * @brief Exception thrown by DXF reading operations
     * 
     * Custom exception class for DXF parsing errors, providing context-specific
     * error messages with "DXF Reader Error:" prefix.
     */
    class DXFReaderException : public std::runtime_error {
    public:
        explicit DXFReaderException(const std::string& message)
            : std::runtime_error("DXF Reader Error: " + message) {}
    };

    /**
     * @brief High-performance DXF file parser for extracting 3D mesh data
     * 
     * The DXFReader class provides robust parsing of AutoCAD DXF files, specifically
     * designed to extract 3DFACE entities and convert them to triangular mesh data.
     * 
     * Key features:
     * - Handles large DXF files (tested with 2900+ entities)
     * - Progress reporting for long-running operations
     * - Cross-platform compatibility (Windows, Linux, macOS)
     * - Memory-efficient line-based parsing
     * - Comprehensive error handling
     * 
     * Usage:
     * @code
     * auto reader = std::make_unique<DXFReader>();
     * reader->setProgressCallback([](double p) { std::cout << (p*100) << "%\n"; });
     * auto meshData = reader->readFile("model.dxf");
     * std::cout << "Found " << meshData->getTriangleCount() << " triangles\n";
     * @endcode
     */
    class DXFReader {
    public:
        DXFReader() = default;
        virtual ~DXFReader() = default;
        
        /**
         * @brief Reads and parses a DXF file to extract 3D mesh data
         * 
         * Main entry point for DXF file processing. Validates file existence
         * and readability before parsing all 3DFACE entities into triangular mesh data.
         * 
         * @param filePath Path to the DXF file to process
         * @return std::unique_ptr<MeshData> Parsed mesh containing all triangles
         * @throws DXFReaderException if file doesn't exist, can't be read, or parsing fails
         */
        std::unique_ptr<MeshData> readFile(const std::string& filePath);
        
        /**
         * @brief Sets callback function for progress reporting
         * 
         * The callback receives progress values from 0.0 (start) to 1.0 (complete)
         * and is called periodically during parsing operations.
         * 
         * @param callback Function to call with progress updates (0.0 to 1.0)
         */
        void setProgressCallback(std::function<void(double)> callback) {
            progressCallback_ = callback;
        }
        
        /**
         * @brief Gets the number of entities processed in the last parsing operation
         * 
         * @return Number of 3DFACE entities successfully parsed
         */
        size_t getLastEntityCount() const { return lastEntityCount_; }
        
    protected:
        /**
         * @brief Internal parsing implementation (can be overridden in derived classes)
         * 
         * @param filePath Path to DXF file (already validated)
         * @return Parsed mesh data
         */
        virtual std::unique_ptr<MeshData> parseFile(const std::string& filePath);
        
    private:
        /**
         * @brief Represents a DXF code-value pair
         * 
         * DXF files consist of alternating code and value lines.
         * Codes indicate the type of data, values contain the actual data.
         */
        struct DXFCode {
            int code;           ///< DXF group code (e.g., 10 for X coordinate)
            std::string value;  ///< Associated value
        };
        
        bool readNextCode(std::ifstream& file, DXFCode& code);
        bool parse3DFace(std::ifstream& file, Triangle& triangle);
        bool parse3DFaceSimple(std::ifstream& file, Triangle& triangle);
        bool parse3DFaceStreamlined(std::ifstream& file, Triangle& triangle);
        bool parse3DFaceFromLines(const std::vector<std::string>& lines, size_t& index, Triangle& triangle);
        Point3D parsePoint(std::ifstream& file, int xCode, int yCode, int zCode);
        
        void reportProgress(double progress);
        
        std::function<void(double)> progressCallback_;
        size_t lastEntityCount_ = 0;
    };

    /**
     * @brief Factory class for creating DXFReader instances
     * 
     * Provides static methods to create DXFReader objects with different
     * configurations. Useful for dependency injection and testing.
     */
    class DXFReaderFactory {
    public:
        /**
         * @brief Creates a standard DXF reader instance
         * 
         * @return std::unique_ptr<DXFReader> Ready-to-use DXF reader
         */
        static std::unique_ptr<DXFReader> createReader() {
            return std::make_unique<DXFReader>();
        }
        
        /**
         * @brief Creates a DXF reader of the specified type
         * 
         * @param readerType Type of reader to create ("standard" or empty for default)
         * @return std::unique_ptr<DXFReader> Configured DXF reader
         * @throws DXFReaderException if readerType is not recognized
         */
        static std::unique_ptr<DXFReader> createReader(const std::string& readerType) {
            if (readerType == "standard" || readerType.empty()) {
                return std::make_unique<DXFReader>();
            }
            throw DXFReaderException("Unknown reader type: " + readerType);
        }
    };

} // namespace DXFProcessor