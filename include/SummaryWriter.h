#pragma once

#include "MeshSummarizer.h"
#include <string>
#include <memory>
#include <filesystem>

namespace DXFProcessor {

    class SummaryWriterException : public std::runtime_error {
    public:
        explicit SummaryWriterException(const std::string& message)
            : std::runtime_error("Summary Writer Error: " + message) {}
    };

    class SummaryWriter {
    public:
        enum class OutputFormat {
            JSON,
            TEXT,
            CSV
        };
        
        SummaryWriter(OutputFormat format = OutputFormat::JSON, const std::string& outputDir = ".");
        virtual ~SummaryWriter() = default;
        
        std::string writeToFile(const MeshSummary& summary, const std::string& baseName = "mesh_summary");
        
        void setOutputDirectory(const std::string& directory);
        void setFormat(OutputFormat format);
        void setIncludeTimestamp(bool include) { includeTimestamp_ = include; }
        void setPrettyPrint(bool pretty) { prettyPrint_ = pretty; }
        
        std::string getLastOutputPath() const { return lastOutputPath_; }
        
    protected:
        virtual std::string formatAsJSON(const MeshSummary& summary);
        virtual std::string formatAsText(const MeshSummary& summary);
        virtual std::string formatAsCSV(const MeshSummary& summary);
        
        std::string generateFilename(const std::string& baseName, const std::string& extension);
        void ensureOutputDirectoryExists();
        
    private:
        OutputFormat format_;
        std::filesystem::path outputDirectory_;
        bool includeTimestamp_;
        bool prettyPrint_;
        std::string lastOutputPath_;
        
        std::string getCurrentTimestamp();
        std::string getFileExtension(OutputFormat format);
        void validateOutputDirectory(const std::filesystem::path& path);
    };

    class JSONSummaryWriter : public SummaryWriter {
    public:
        JSONSummaryWriter(const std::string& outputDir = ".") 
            : SummaryWriter(OutputFormat::JSON, outputDir) {}
    };
    
    class TextSummaryWriter : public SummaryWriter {
    public:
        TextSummaryWriter(const std::string& outputDir = ".") 
            : SummaryWriter(OutputFormat::TEXT, outputDir) {}
    };
    
    class CSVSummaryWriter : public SummaryWriter {
    public:
        CSVSummaryWriter(const std::string& outputDir = ".") 
            : SummaryWriter(OutputFormat::CSV, outputDir) {}
    };

    class SummaryWriterFactory {
    public:
        static std::unique_ptr<SummaryWriter> create(
            SummaryWriter::OutputFormat format = SummaryWriter::OutputFormat::JSON,
            const std::string& outputDir = ".") {
            return std::make_unique<SummaryWriter>(format, outputDir);
        }
        
        static std::unique_ptr<SummaryWriter> create(
            const std::string& formatName,
            const std::string& outputDir = ".") {
            
            SummaryWriter::OutputFormat format;
            if (formatName == "json" || formatName.empty()) {
                format = SummaryWriter::OutputFormat::JSON;
            } else if (formatName == "text" || formatName == "txt") {
                format = SummaryWriter::OutputFormat::TEXT;
            } else if (formatName == "csv") {
                format = SummaryWriter::OutputFormat::CSV;
            } else {
                format = SummaryWriter::OutputFormat::JSON;
            }
            
            return create(format, outputDir);
        }
    };

} // namespace DXFProcessor