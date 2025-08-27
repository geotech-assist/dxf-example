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

    std::unique_ptr<MeshData> DXFReader::readFile(const std::string& filePath) {
        if (!std::filesystem::exists(filePath)) {
            throw DXFReaderException("File does not exist: " + filePath);
        }
        
        if (!std::filesystem::is_regular_file(filePath)) {
            throw DXFReaderException("Path is not a regular file: " + filePath);
        }
        
        return parseFile(filePath);
    }

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

    void DXFReader::reportProgress(double progress) {
        if (progressCallback_) {
            progressCallback_(std::max(0.0, std::min(1.0, progress)));
        }
    }

} // namespace DXFProcessor