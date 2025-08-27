#include "DXFReader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <filesystem>
#include <limits>
#include <cmath>
#include <vector>

namespace DXFProcessor {

    /**
     * @brief Reads and parses a DXF file to extract 3D mesh data
     * 
     * This is the main entry point for DXF file processing. It validates the file
     * exists and is readable before delegating to the internal parser.
     * 
     * @param filePath Path to the DXF file to process
     * @return std::unique_ptr<MeshData> Parsed mesh data containing triangles
     * @throws DXFReaderException if file doesn't exist, isn't readable, or parsing fails
     */
    std::unique_ptr<MeshData> DXFReader::readFile(const std::string& filePath) {
        if (!std::filesystem::exists(filePath)) {
            throw DXFReaderException("File does not exist: " + filePath);
        }
        
        if (!std::filesystem::is_regular_file(filePath)) {
            throw DXFReaderException("Path is not a regular file: " + filePath);
        }
        
        return parseFile(filePath);
    }

    /**
     * @brief Internal parser that processes DXF file content line by line
     * 
     * Uses a robust line-based parsing approach that reads the entire file into memory
     * first, then processes it sequentially. This eliminates file seeking issues and
     * provides reliable parsing of DXF code-value pairs.
     * 
     * @param filePath Path to the DXF file (already validated)
     * @return std::unique_ptr<MeshData> Mesh data with all parsed 3DFACE entities
     * @throws DXFReaderException if file cannot be opened or parsing fails
     */
    std::unique_ptr<MeshData> DXFReader::parseFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw DXFReaderException("Cannot open file: " + filePath);
        }
        
        auto meshData = std::make_unique<MeshData>();
        lastEntityCount_ = 0;
        
        size_t fileSize = std::filesystem::file_size(filePath);
        
        meshData->reserve(3000);
        
        std::vector<std::string> lines;
        std::string line;
        
        // Read all lines first
        while (std::getline(file, line)) {
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            line.erase(0, line.find_first_not_of(" \t"));
            lines.push_back(line);
        }
        
        file.close();
        
        bool inEntitiesSection = false;
        size_t i = 0;
        
        try {
            while (i < lines.size()) {
                if (lines[i] == "0" && i + 1 < lines.size()) {
                    std::string entityType = lines[i + 1];
                    
                    if (entityType == "SECTION" && i + 3 < lines.size()) {
                        if (lines[i + 2] == "2" && lines[i + 3] == "ENTITIES") {
                            inEntitiesSection = true;
                            i += 4;
                            continue;
                        }
                    } else if (entityType == "ENDSEC") {
                        inEntitiesSection = false;
                        i += 2;
                        continue;
                    } else if (inEntitiesSection && entityType == "3DFACE") {
                        Triangle triangle;
                        size_t nextEntityIndex = i + 2;
                        if (parse3DFaceFromLines(lines, nextEntityIndex, triangle)) {
                            meshData->addTriangle(triangle);
                            lastEntityCount_++;
                            
                            if (lastEntityCount_ % 100 == 0 && fileSize > 0) {
                                double progress = static_cast<double>(nextEntityIndex) / lines.size();
                                reportProgress(progress);
                            }
                        }
                        i = nextEntityIndex;
                        continue;
                    }
                }
                i++;
            }
        } catch (const std::exception& e) {
            throw DXFReaderException("Parse error: " + std::string(e.what()));
        }
        
        reportProgress(1.0);
        
        if (meshData->isEmpty()) {
            throw DXFReaderException("No 3D faces found in DXF file");
        }
        
        return meshData;
    }

    /**
     * @brief Reads the next DXF code-value pair from file stream
     * 
     * DXF files consist of code-value pairs where the code (integer) appears on one line
     * and its corresponding value appears on the next line.
     * 
     * @param file Input file stream positioned at a code line
     * @param code Reference to DXFCode structure to populate
     * @return true if code-value pair was successfully read, false on EOF or error
     */
    bool DXFReader::readNextCode(std::ifstream& file, DXFCode& code) {
        std::string line;
        
        if (!std::getline(file, line)) {
            return false;
        }
        
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        try {
            code.code = std::stoi(line);
        } catch (const std::exception&) {
            return false;
        }
        
        if (!std::getline(file, line)) {
            return false;
        }
        
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        code.value = line;
        
        return true;
    }

    /**
     * @brief Legacy 3DFACE parser using file stream (deprecated)
     * 
     * This method uses file stream operations with seeking, which proved unreliable
     * for complex DXF files. Kept for compatibility but not used in current implementation.
     * 
     * @param file Input file stream positioned after 3DFACE entity marker
     * @param triangle Reference to Triangle to populate with vertex data
     * @return true if triangle was successfully parsed
     * @deprecated Use parse3DFaceFromLines instead
     */
    bool DXFReader::parse3DFace(std::ifstream& file, Triangle& triangle) {
        DXFCode code;
        Point3D vertices[4];
        int vertexCount = 0;
        
        while (readNextCode(file, code)) {
            if (code.code == 0) {
                // We've reached the next entity, put this code back
                std::string codeStr = std::to_string(code.code);
                int bytesToGoBack = codeStr.length() + 1 + code.value.length() + 1;
                file.seekg(-bytesToGoBack, std::ios::cur);
                break;
            }
            
            switch (code.code) {
                case 10:
                case 11:
                case 12:
                case 13: {
                    int vertexIndex = code.code - 10;
                    if (vertexIndex < 4) {
                        try {
                            vertices[vertexIndex].x = std::stod(code.value);
                        } catch (const std::exception&) {
                            vertices[vertexIndex].x = 0.0;
                        }
                    }
                    break;
                }
                case 20:
                case 21:
                case 22:
                case 23: {
                    int vertexIndex = code.code - 20;
                    if (vertexIndex < 4) {
                        try {
                            vertices[vertexIndex].y = std::stod(code.value);
                        } catch (const std::exception&) {
                            vertices[vertexIndex].y = 0.0;
                        }
                    }
                    break;
                }
                case 30:
                case 31:
                case 32:
                case 33: {
                    int vertexIndex = code.code - 30;
                    if (vertexIndex < 4) {
                        try {
                            vertices[vertexIndex].z = std::stod(code.value);
                            vertexCount = std::max(vertexCount, vertexIndex + 1);
                        } catch (const std::exception&) {
                            vertices[vertexIndex].z = 0.0;
                        }
                    }
                    break;
                }
            }
        }
        
        if (vertexCount >= 3) {
            triangle = Triangle(vertices[0], vertices[1], vertices[2]);
            return true;
        }
        
        return false;
    }

    /**
     * @brief Simplified 3DFACE parser with basic error handling
     * 
     * An intermediate parsing approach that improved upon the original but still
     * suffered from file positioning issues. Kept for reference.
     * 
     * @param file Input file stream positioned after 3DFACE entity marker
     * @param triangle Reference to Triangle to populate with vertex data
     * @return true if triangle was successfully parsed
     * @deprecated Use parse3DFaceFromLines instead
     */
    bool DXFReader::parse3DFaceSimple(std::ifstream& file, Triangle& triangle) {
        std::string line;
        Point3D vertices[4];
        bool hasVertices[4] = {false, false, false, false};
        
        // Read until next entity (code 0)
        while (std::getline(file, line)) {
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            line.erase(0, line.find_first_not_of(" \t"));
            
            // Check if it's a code line (should be a number)
            if (line.empty()) continue;
            
            try {
                int code = std::stoi(line);
                
                if (code == 0) {
                    // Put this line back by seeking back
                    file.seekg(-static_cast<int>(line.length() + 3), std::ios::cur);
                    break;
                }
                
                // Read the value line
                if (!std::getline(file, line)) break;
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                line.erase(0, line.find_first_not_of(" \t"));
                
                // Parse vertex coordinates
                switch (code) {
                    case 10: case 11: case 12: case 13: {
                        int vertexIndex = code - 10;
                        if (vertexIndex < 4) {
                            try {
                                vertices[vertexIndex].x = std::stod(line);
                                hasVertices[vertexIndex] = true;
                            } catch (const std::exception&) {
                                vertices[vertexIndex].x = 0.0;
                            }
                        }
                        break;
                    }
                    case 20: case 21: case 22: case 23: {
                        int vertexIndex = code - 20;
                        if (vertexIndex < 4) {
                            try {
                                vertices[vertexIndex].y = std::stod(line);
                            } catch (const std::exception&) {
                                vertices[vertexIndex].y = 0.0;
                            }
                        }
                        break;
                    }
                    case 30: case 31: case 32: case 33: {
                        int vertexIndex = code - 30;
                        if (vertexIndex < 4) {
                            try {
                                vertices[vertexIndex].z = std::stod(line);
                            } catch (const std::exception&) {
                                vertices[vertexIndex].z = 0.0;
                            }
                        }
                        break;
                    }
                }
            } catch (const std::exception&) {
                // Not a valid code, continue
            }
        }
        
        // Check if we have at least 3 vertices
        if (hasVertices[0] && hasVertices[1] && hasVertices[2]) {
            triangle = Triangle(vertices[0], vertices[1], vertices[2]);
            return true;
        }
        
        return false;
    }

    /**
     * @brief Streamlined 3DFACE parser with improved line handling
     * 
     * Another iteration of the parsing logic that attempted to fix file positioning
     * issues but still had reliability problems with large files.
     * 
     * @param file Input file stream positioned after 3DFACE entity marker
     * @param triangle Reference to Triangle to populate with vertex data
     * @return true if triangle was successfully parsed
     * @deprecated Use parse3DFaceFromLines instead
     */
    bool DXFReader::parse3DFaceStreamlined(std::ifstream& file, Triangle& triangle) {
        std::string line;
        Point3D vertices[3];
        int vertexCount = 0;
        
        // Read until next entity (code 0) or end of file
        while (std::getline(file, line)) {
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            line.erase(0, line.find_first_not_of(" \t"));
            
            if (line.empty()) continue;
            
            try {
                int code = std::stoi(line);
                
                if (code == 0) {
                    // Put this line back for main parser
                    file.seekg(-static_cast<int>(line.length() + 1), std::ios::cur);
                    break;
                }
                
                // Read the value line
                if (!std::getline(file, line)) break;
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                line.erase(0, line.find_first_not_of(" \t"));
                
                // Parse vertex coordinates for first 3 vertices only
                switch (code) {
                    case 10: case 11: case 12: {
                        int vertexIndex = code - 10;
                        if (vertexIndex < 3) {
                            try {
                                vertices[vertexIndex].x = std::stod(line);
                            } catch (const std::exception&) {
                                vertices[vertexIndex].x = 0.0;
                            }
                        }
                        break;
                    }
                    case 20: case 21: case 22: {
                        int vertexIndex = code - 20;
                        if (vertexIndex < 3) {
                            try {
                                vertices[vertexIndex].y = std::stod(line);
                            } catch (const std::exception&) {
                                vertices[vertexIndex].y = 0.0;
                            }
                        }
                        break;
                    }
                    case 30: case 31: case 32: {
                        int vertexIndex = code - 30;
                        if (vertexIndex < 3) {
                            try {
                                vertices[vertexIndex].z = std::stod(line);
                                vertexCount = std::max(vertexCount, vertexIndex + 1);
                            } catch (const std::exception&) {
                                vertices[vertexIndex].z = 0.0;
                            }
                        }
                        break;
                    }
                }
            } catch (const std::exception&) {
                // Not a valid code, skip
            }
        }
        
        if (vertexCount >= 3) {
            triangle = Triangle(vertices[0], vertices[1], vertices[2]);
            return true;
        }
        
        return false;
    }

    /**
     * @brief Robust 3DFACE parser using pre-loaded lines array
     * 
     * This is the current production parser that processes 3DFACE entities from
     * a pre-loaded array of file lines. It correctly handles DXF code-value pairs
     * and extracts the first three vertices to form a triangle.
     * 
     * DXF 3DFACE format:
     * - Codes 10,11,12,13: X coordinates for vertices 0,1,2,3
     * - Codes 20,21,22,23: Y coordinates for vertices 0,1,2,3  
     * - Codes 30,31,32,33: Z coordinates for vertices 0,1,2,3
     * 
     * @param lines Vector of all file lines (trimmed)
     * @param index Reference to current line index, updated to point past this entity
     * @param triangle Reference to Triangle to populate with vertex data
     * @return true if triangle was successfully parsed with all 3 vertices
     */
    bool DXFReader::parse3DFaceFromLines(const std::vector<std::string>& lines, size_t& index, Triangle& triangle) {
        Point3D vertices[3];
        bool hasVertex[3] = {false, false, false};
        
        // Read until next "0" code or end of lines
        while (index < lines.size()) {
            if (lines[index] == "0") {
                break; // Next entity found
            }
            
            try {
                int code = std::stoi(lines[index]);
                
                if (index + 1 >= lines.size()) break;
                std::string value = lines[index + 1];
                
                // Parse vertex coordinates for first 3 vertices
                switch (code) {
                    case 10: case 11: case 12: {
                        int vertexIndex = code - 10;
                        if (vertexIndex < 3) {
                            try {
                                vertices[vertexIndex].x = std::stod(value);
                                hasVertex[vertexIndex] = true;
                            } catch (const std::exception&) {
                                vertices[vertexIndex].x = 0.0;
                            }
                        }
                        break;
                    }
                    case 20: case 21: case 22: {
                        int vertexIndex = code - 20;
                        if (vertexIndex < 3) {
                            try {
                                vertices[vertexIndex].y = std::stod(value);
                            } catch (const std::exception&) {
                                vertices[vertexIndex].y = 0.0;
                            }
                        }
                        break;
                    }
                    case 30: case 31: case 32: {
                        int vertexIndex = code - 30;
                        if (vertexIndex < 3) {
                            try {
                                vertices[vertexIndex].z = std::stod(value);
                            } catch (const std::exception&) {
                                vertices[vertexIndex].z = 0.0;
                            }
                        }
                        break;
                    }
                }
                
                index += 2; // Skip code and value
            } catch (const std::exception&) {
                // Not a valid code, skip
                index++;
            }
        }
        
        // Check if we have all 3 vertices
        if (hasVertex[0] && hasVertex[1] && hasVertex[2]) {
            triangle = Triangle(vertices[0], vertices[1], vertices[2]);
            return true;
        }
        
        return false;
    }

    /**
     * @brief Parses a 3D point from DXF code-value pairs
     * 
     * Utility function for extracting 3D coordinates from specific DXF codes.
     * Used by legacy parsers but not in the current implementation.
     * 
     * @param file Input file stream
     * @param xCode DXF code for X coordinate
     * @param yCode DXF code for Y coordinate  
     * @param zCode DXF code for Z coordinate
     * @return Point3D with extracted coordinates
     * @deprecated Not used in current line-based parser
     */
    Point3D DXFReader::parsePoint(std::ifstream& file, int xCode, int yCode, int zCode) {
        Point3D point;
        DXFCode code;
        
        while (readNextCode(file, code) && code.code != 0) {
            if (code.code == xCode) {
                try {
                    point.x = std::stod(code.value);
                } catch (const std::exception&) {
                    point.x = 0.0;
                }
            } else if (code.code == yCode) {
                try {
                    point.y = std::stod(code.value);
                } catch (const std::exception&) {
                    point.y = 0.0;
                }
            } else if (code.code == zCode) {
                try {
                    point.z = std::stod(code.value);
                } catch (const std::exception&) {
                    point.z = 0.0;
                }
            }
        }
        
        return point;
    }

    /**
     * @brief Reports parsing progress to registered callback
     * 
     * Notifies the progress callback (if set) about parsing progress.
     * Progress is clamped to [0.0, 1.0] range.
     * 
     * @param progress Progress value between 0.0 (start) and 1.0 (complete)
     */
    void DXFReader::reportProgress(double progress) {
        if (progressCallback_) {
            progressCallback_(std::max(0.0, std::min(1.0, progress)));
        }
    }

} // namespace DXFProcessor