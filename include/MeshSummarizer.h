#pragma once

#include "MeshData.h"
#include <string>
#include <map>
#include <memory>

namespace DXFProcessor {

    struct MeshSummary {
        size_t triangleCount = 0;
        BoundingBox boundingBox;
        double totalSurfaceArea = 0.0;
        Point3D centroid;
        
        std::map<std::string, std::string> customFields;
        
        void addCustomField(const std::string& key, const std::string& value) {
            customFields[key] = value;
        }
        
        std::string getCustomField(const std::string& key) const {
            auto it = customFields.find(key);
            return (it != customFields.end()) ? it->second : "";
        }
    };

    class MeshSummarizer {
    public:
        MeshSummarizer() = default;
        virtual ~MeshSummarizer() = default;
        
        virtual MeshSummary summarize(const MeshData& meshData);
        
    protected:
        virtual void calculateBasicStats(const MeshData& meshData, MeshSummary& summary);
        virtual void calculateAdvancedStats(const MeshData& meshData, MeshSummary& summary);
        virtual void addCustomCalculations(const MeshData& meshData, MeshSummary& summary);
        
    private:
        Point3D calculateCentroid(const MeshData& meshData);
    };

    class DetailedMeshSummarizer : public MeshSummarizer {
    public:
        DetailedMeshSummarizer() = default;
        
    protected:
        void addCustomCalculations(const MeshData& meshData, MeshSummary& summary) override;
        
    private:
        double calculateVolume(const MeshData& meshData);
        double calculateAverageTriangleArea(const MeshData& meshData);
        std::pair<double, double> getTriangleAreaRange(const MeshData& meshData);
    };

    class MeshSummarizerFactory {
    public:
        enum class SummarizerType {
            Basic,
            Detailed
        };
        
        static std::unique_ptr<MeshSummarizer> create(SummarizerType type = SummarizerType::Basic) {
            switch (type) {
                case SummarizerType::Basic:
                    return std::make_unique<MeshSummarizer>();
                case SummarizerType::Detailed:
                    return std::make_unique<DetailedMeshSummarizer>();
                default:
                    return std::make_unique<MeshSummarizer>();
            }
        }
        
        static std::unique_ptr<MeshSummarizer> create(const std::string& typeName) {
            if (typeName == "basic" || typeName.empty()) {
                return create(SummarizerType::Basic);
            } else if (typeName == "detailed") {
                return create(SummarizerType::Detailed);
            }
            return create(SummarizerType::Basic);
        }
    };

} // namespace DXFProcessor