#pragma once

#include "MeshData.h"
#include <string>
#include <memory>
#include <stdexcept>
#include <functional>

namespace DXFProcessor {

    class DXFReaderException : public std::runtime_error {
    public:
        explicit DXFReaderException(const std::string& message)
            : std::runtime_error("DXF Reader Error: " + message) {}
    };

    class DXFReader {
    public:
        DXFReader() = default;
        virtual ~DXFReader() = default;
        
        std::unique_ptr<MeshData> readFile(const std::string& filePath);
        
        void setProgressCallback(std::function<void(double)> callback) {
            progressCallback_ = callback;
        }
        
        size_t getLastEntityCount() const { return lastEntityCount_; }
        
    protected:
        virtual std::unique_ptr<MeshData> parseFile(const std::string& filePath);
        
    private:
        struct DXFCode {
            int code;
            std::string value;
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

    class DXFReaderFactory {
    public:
        static std::unique_ptr<DXFReader> createReader() {
            return std::make_unique<DXFReader>();
        }
        
        static std::unique_ptr<DXFReader> createReader(const std::string& readerType) {
            if (readerType == "standard" || readerType.empty()) {
                return std::make_unique<DXFReader>();
            }
            throw DXFReaderException("Unknown reader type: " + readerType);
        }
    };

} // namespace DXFProcessor