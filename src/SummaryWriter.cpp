#include "SummaryWriter.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

namespace DXFProcessor {

    SummaryWriter::SummaryWriter(OutputFormat format, const std::string& outputDir) 
        : format_(format)
        , outputDirectory_(outputDir)
        , includeTimestamp_(true)
        , prettyPrint_(true) {
        ensureOutputDirectoryExists();
    }

    std::string SummaryWriter::writeToFile(const MeshSummary& summary, const std::string& baseName) {
        ensureOutputDirectoryExists();
        
        std::string content;
        std::string extension = getFileExtension(format_);
        
        switch (format_) {
            case OutputFormat::JSON:
                content = formatAsJSON(summary);
                break;
            case OutputFormat::TEXT:
                content = formatAsText(summary);
                break;
            case OutputFormat::CSV:
                content = formatAsCSV(summary);
                break;
        }
        
        std::string filename = generateFilename(baseName, extension);
        std::filesystem::path fullPath = outputDirectory_ / filename;
        
        std::ofstream file(fullPath);
        if (!file.is_open()) {
            throw SummaryWriterException("Cannot create output file: " + fullPath.string());
        }
        
        file << content;
        file.close();
        
        lastOutputPath_ = std::filesystem::absolute(fullPath).string();
        return lastOutputPath_;
    }

    void SummaryWriter::setOutputDirectory(const std::string& directory) {
        outputDirectory_ = directory;
        validateOutputDirectory(outputDirectory_);
        ensureOutputDirectoryExists();
    }

    void SummaryWriter::setFormat(OutputFormat format) {
        format_ = format;
    }

    std::string SummaryWriter::formatAsJSON(const MeshSummary& summary) {
        std::ostringstream json;
        
        if (prettyPrint_) {
            json << "{\n";
            json << "  \"triangle_count\": " << summary.triangleCount << ",\n";
            json << "  \"total_surface_area\": " << std::fixed << std::setprecision(6) 
                 << summary.totalSurfaceArea << ",\n";
            
            json << "  \"bounding_box\": {\n";
            json << "    \"min\": {\n";
            json << "      \"x\": " << summary.boundingBox.min.x << ",\n";
            json << "      \"y\": " << summary.boundingBox.min.y << ",\n";
            json << "      \"z\": " << summary.boundingBox.min.z << "\n";
            json << "    },\n";
            json << "    \"max\": {\n";
            json << "      \"x\": " << summary.boundingBox.max.x << ",\n";
            json << "      \"y\": " << summary.boundingBox.max.y << ",\n";
            json << "      \"z\": " << summary.boundingBox.max.z << "\n";
            json << "    },\n";
            json << "    \"size\": {\n";
            Point3D size = summary.boundingBox.size();
            json << "      \"width\": " << size.x << ",\n";
            json << "      \"height\": " << size.y << ",\n";
            json << "      \"depth\": " << size.z << "\n";
            json << "    }\n";
            json << "  },\n";
            
            json << "  \"centroid\": {\n";
            json << "    \"x\": " << summary.centroid.x << ",\n";
            json << "    \"y\": " << summary.centroid.y << ",\n";
            json << "    \"z\": " << summary.centroid.z << "\n";
            json << "  }";
            
            if (!summary.customFields.empty()) {
                json << ",\n  \"custom_fields\": {\n";
                size_t count = 0;
                for (const auto& [key, value] : summary.customFields) {
                    json << "    \"" << key << "\": ";
                    
                    try {
                        double numValue = std::stod(value);
                        json << std::fixed << std::setprecision(6) << numValue;
                    } catch (const std::exception&) {
                        json << "\"" << value << "\"";
                    }
                    
                    if (++count < summary.customFields.size()) {
                        json << ",";
                    }
                    json << "\n";
                }
                json << "  }";
            }
            
            if (includeTimestamp_) {
                json << ",\n  \"timestamp\": \"" << getCurrentTimestamp() << "\"";
            }
            
            json << "\n}";
        } else {
            json << "{\"triangle_count\":" << summary.triangleCount
                 << ",\"total_surface_area\":" << summary.totalSurfaceArea
                 << ",\"bounding_box\":{\"min\":{\"x\":" << summary.boundingBox.min.x
                 << ",\"y\":" << summary.boundingBox.min.y << ",\"z\":" << summary.boundingBox.min.z
                 << "},\"max\":{\"x\":" << summary.boundingBox.max.x
                 << ",\"y\":" << summary.boundingBox.max.y << ",\"z\":" << summary.boundingBox.max.z
                 << "}},\"centroid\":{\"x\":" << summary.centroid.x
                 << ",\"y\":" << summary.centroid.y << ",\"z\":" << summary.centroid.z << "}}";
        }
        
        return json.str();
    }

    std::string SummaryWriter::formatAsText(const MeshSummary& summary) {
        std::ostringstream text;
        
        text << "DXF Mesh Summary\n";
        text << "================\n\n";
        
        if (includeTimestamp_) {
            text << "Generated: " << getCurrentTimestamp() << "\n\n";
        }
        
        text << "Basic Statistics:\n";
        text << "-----------------\n";
        text << "Triangle Count: " << summary.triangleCount << "\n";
        text << "Total Surface Area: " << std::fixed << std::setprecision(6) 
             << summary.totalSurfaceArea << "\n\n";
        
        text << "Bounding Box:\n";
        text << "-------------\n";
        text << "Min Point: (" << summary.boundingBox.min.x << ", " 
             << summary.boundingBox.min.y << ", " << summary.boundingBox.min.z << ")\n";
        text << "Max Point: (" << summary.boundingBox.max.x << ", " 
             << summary.boundingBox.max.y << ", " << summary.boundingBox.max.z << ")\n";
        
        Point3D size = summary.boundingBox.size();
        text << "Dimensions: " << size.x << " x " << size.y << " x " << size.z << "\n";
        text << "Volume: " << summary.boundingBox.volume() << "\n\n";
        
        text << "Centroid: (" << summary.centroid.x << ", " 
             << summary.centroid.y << ", " << summary.centroid.z << ")\n\n";
        
        if (!summary.customFields.empty()) {
            text << "Additional Properties:\n";
            text << "---------------------\n";
            for (const auto& [key, value] : summary.customFields) {
                text << key << ": " << value << "\n";
            }
        }
        
        return text.str();
    }

    std::string SummaryWriter::formatAsCSV(const MeshSummary& summary) {
        std::ostringstream csv;
        
        csv << "Property,Value\n";
        csv << "triangle_count," << summary.triangleCount << "\n";
        csv << "total_surface_area," << std::fixed << std::setprecision(6) 
            << summary.totalSurfaceArea << "\n";
        csv << "bounding_box_min_x," << summary.boundingBox.min.x << "\n";
        csv << "bounding_box_min_y," << summary.boundingBox.min.y << "\n";
        csv << "bounding_box_min_z," << summary.boundingBox.min.z << "\n";
        csv << "bounding_box_max_x," << summary.boundingBox.max.x << "\n";
        csv << "bounding_box_max_y," << summary.boundingBox.max.y << "\n";
        csv << "bounding_box_max_z," << summary.boundingBox.max.z << "\n";
        
        Point3D size = summary.boundingBox.size();
        csv << "width," << size.x << "\n";
        csv << "height," << size.y << "\n";
        csv << "depth," << size.z << "\n";
        csv << "volume," << summary.boundingBox.volume() << "\n";
        
        csv << "centroid_x," << summary.centroid.x << "\n";
        csv << "centroid_y," << summary.centroid.y << "\n";
        csv << "centroid_z," << summary.centroid.z << "\n";
        
        for (const auto& [key, value] : summary.customFields) {
            csv << key << "," << value << "\n";
        }
        
        if (includeTimestamp_) {
            csv << "timestamp," << getCurrentTimestamp() << "\n";
        }
        
        return csv.str();
    }

    std::string SummaryWriter::generateFilename(const std::string& baseName, const std::string& extension) {
        std::ostringstream filename;
        filename << baseName;
        
        if (includeTimestamp_) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            filename << "_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
            filename << "_" << std::setfill('0') << std::setw(3) << ms.count();
        }
        
        filename << extension;
        return filename.str();
    }

    void SummaryWriter::ensureOutputDirectoryExists() {
        if (!std::filesystem::exists(outputDirectory_)) {
            try {
                std::filesystem::create_directories(outputDirectory_);
            } catch (const std::filesystem::filesystem_error& e) {
                throw SummaryWriterException("Cannot create output directory: " + std::string(e.what()));
            }
        }
        validateOutputDirectory(outputDirectory_);
    }

    std::string SummaryWriter::getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream timestamp;
        timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        return timestamp.str();
    }

    std::string SummaryWriter::getFileExtension(OutputFormat format) {
        switch (format) {
            case OutputFormat::JSON: return ".json";
            case OutputFormat::TEXT: return ".txt";
            case OutputFormat::CSV: return ".csv";
            default: return ".txt";
        }
    }

    void SummaryWriter::validateOutputDirectory(const std::filesystem::path& path) {
        if (std::filesystem::exists(path) && !std::filesystem::is_directory(path)) {
            throw SummaryWriterException("Output path exists but is not a directory: " + path.string());
        }
    }

} // namespace DXFProcessor