/**
 * @file test_summary_writer.cpp
 * @brief Unit tests for SummaryWriter functionality
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "SummaryWriter.h"
#include "MeshSummarizer.h"
#include "MeshData.h"
#include <filesystem>
#include <fstream>
#include <regex>

using namespace DXFProcessor;

class SummaryWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test output directory
        testOutputDir = "test_output";
        std::filesystem::create_directories(testOutputDir);
        
        // Create sample mesh data and summary
        meshData = std::make_unique<MeshData>();
        meshData->addTriangle(
            Point3D(0.0, 0.0, 0.0),
            Point3D(1.0, 0.0, 0.0),
            Point3D(0.0, 1.0, 0.0)
        );
        
        auto summarizer = MeshSummarizerFactory::create();
        testSummary = summarizer->summarize(*meshData);
        testSummary.addCustomField("test_field", "test_value");
    }
    
    void TearDown() override {
        // Clean up test output directory
        if (std::filesystem::exists(testOutputDir)) {
            std::filesystem::remove_all(testOutputDir);
        }
    }
    
    std::string readFileContents(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return "";
        }
        
        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        return content;
    }
    
    std::unique_ptr<MeshData> meshData;
    MeshSummary testSummary;
    std::string testOutputDir;
};

TEST_F(SummaryWriterTest, FactoryCreateJSONWriter) {
    auto writer = SummaryWriterFactory::create(SummaryWriter::OutputFormat::JSON, testOutputDir);
    EXPECT_NE(writer, nullptr);
}

TEST_F(SummaryWriterTest, FactoryCreateFromString) {
    auto jsonWriter = SummaryWriterFactory::create("json", testOutputDir);
    auto textWriter = SummaryWriterFactory::create("text", testOutputDir);
    auto csvWriter = SummaryWriterFactory::create("csv", testOutputDir);
    auto defaultWriter = SummaryWriterFactory::create("", testOutputDir);
    
    EXPECT_NE(jsonWriter, nullptr);
    EXPECT_NE(textWriter, nullptr);
    EXPECT_NE(csvWriter, nullptr);
    EXPECT_NE(defaultWriter, nullptr);
}

TEST_F(SummaryWriterTest, WriteJSONFormat) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);  // For predictable filenames
    
    std::string outputPath = writer->writeToFile(testSummary, "test_summary");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    EXPECT_EQ(writer->getLastOutputPath(), outputPath);
    
    std::string content = readFileContents(outputPath);
    EXPECT_FALSE(content.empty());
    
    // Check for JSON structure
    EXPECT_NE(content.find("{"), std::string::npos);
    EXPECT_NE(content.find("}"), std::string::npos);
    EXPECT_NE(content.find("triangle_count"), std::string::npos);
    EXPECT_NE(content.find("total_surface_area"), std::string::npos);
}

TEST_F(SummaryWriterTest, WriteTextFormat) {
    auto writer = SummaryWriterFactory::create("text", testOutputDir);
    writer->setIncludeTimestamp(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "test_summary");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    
    std::string content = readFileContents(outputPath);
    EXPECT_FALSE(content.empty());
    
    // Check for text format structure
    EXPECT_NE(content.find("DXF Mesh Summary"), std::string::npos);
    EXPECT_NE(content.find("Basic Statistics"), std::string::npos);
    EXPECT_NE(content.find("Triangle Count:"), std::string::npos);
}

TEST_F(SummaryWriterTest, WriteCSVFormat) {
    auto writer = SummaryWriterFactory::create("csv", testOutputDir);
    writer->setIncludeTimestamp(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "test_summary");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    
    std::string content = readFileContents(outputPath);
    EXPECT_FALSE(content.empty());
    
    // Check for CSV structure
    EXPECT_NE(content.find("Property,Value"), std::string::npos);
    EXPECT_NE(content.find("triangle_count,"), std::string::npos);
}

TEST_F(SummaryWriterTest, FileExtensions) {
    auto jsonWriter = SummaryWriterFactory::create("json", testOutputDir);
    auto textWriter = SummaryWriterFactory::create("text", testOutputDir);
    auto csvWriter = SummaryWriterFactory::create("csv", testOutputDir);
    
    jsonWriter->setIncludeTimestamp(false);
    textWriter->setIncludeTimestamp(false);
    csvWriter->setIncludeTimestamp(false);
    
    std::string jsonPath = jsonWriter->writeToFile(testSummary, "test");
    std::string textPath = textWriter->writeToFile(testSummary, "test");
    std::string csvPath = csvWriter->writeToFile(testSummary, "test");
    
    EXPECT_NE(jsonPath.find(".json"), std::string::npos);
    EXPECT_NE(textPath.find(".txt"), std::string::npos);
    EXPECT_NE(csvPath.find(".csv"), std::string::npos);
}

TEST_F(SummaryWriterTest, TimestampInFilename) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(true);
    
    std::string outputPath = writer->writeToFile(testSummary, "timestamped");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    
    // Filename should contain timestamp pattern (YYYYMMDD_HHMMSS_mmm)
    std::regex timestampPattern(R"(timestamped_\d{8}_\d{6}_\d{3}\.json)");
    std::filesystem::path path(outputPath);
    EXPECT_TRUE(std::regex_search(path.filename().string(), timestampPattern));
}

TEST_F(SummaryWriterTest, NoTimestampInFilename) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "no_timestamp");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    
    std::filesystem::path path(outputPath);
    EXPECT_EQ(path.filename().string(), "no_timestamp.json");
}

TEST_F(SummaryWriterTest, PrettyPrintJSON) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);
    writer->setPrettyPrint(true);
    
    std::string outputPath = writer->writeToFile(testSummary, "pretty");
    std::string content = readFileContents(outputPath);
    
    // Pretty printed JSON should have newlines and indentation
    EXPECT_NE(content.find("{\n"), std::string::npos);
    EXPECT_NE(content.find("  \""), std::string::npos);  // Indented fields
}

TEST_F(SummaryWriterTest, CompactJSON) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);
    writer->setPrettyPrint(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "compact");
    std::string content = readFileContents(outputPath);
    
    // Compact JSON should be more condensed
    EXPECT_NE(content.find("{\""), std::string::npos);
}

TEST_F(SummaryWriterTest, CustomFields) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "custom_fields");
    std::string content = readFileContents(outputPath);
    
    // Should contain our custom field
    EXPECT_NE(content.find("test_field"), std::string::npos);
    EXPECT_NE(content.find("test_value"), std::string::npos);
}

TEST_F(SummaryWriterTest, OutputDirectoryCreation) {
    std::string newOutputDir = testOutputDir + "/nested/directory";
    
    auto writer = SummaryWriterFactory::create("json", newOutputDir);
    writer->setIncludeTimestamp(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "nested");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    EXPECT_TRUE(std::filesystem::exists(newOutputDir));
}

TEST_F(SummaryWriterTest, InvalidOutputDirectory) {
    // Try to write to a file as if it were a directory
    std::string invalidDir = testOutputDir + "/file.txt";
    std::ofstream(invalidDir) << "content";  // Create a file
    
    EXPECT_THROW(SummaryWriterFactory::create("json", invalidDir), SummaryWriterException);
}

TEST_F(SummaryWriterTest, ChangeOutputDirectory) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    
    std::string newDir = testOutputDir + "/changed";
    writer->setOutputDirectory(newDir);
    writer->setIncludeTimestamp(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "moved");
    
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    EXPECT_NE(outputPath.find(newDir), std::string::npos);
}

TEST_F(SummaryWriterTest, ChangeFormat) {
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);
    
    // Write as JSON first
    std::string jsonPath = writer->writeToFile(testSummary, "format_test");
    EXPECT_NE(jsonPath.find(".json"), std::string::npos);
    
    // Change to text format
    writer->setFormat(SummaryWriter::OutputFormat::TEXT);
    std::string textPath = writer->writeToFile(testSummary, "format_test");
    EXPECT_NE(textPath.find(".txt"), std::string::npos);
}

TEST_F(SummaryWriterTest, SpecializedWriters) {
    JSONSummaryWriter jsonWriter(testOutputDir);
    TextSummaryWriter textWriter(testOutputDir);
    CSVSummaryWriter csvWriter(testOutputDir);
    
    jsonWriter.setIncludeTimestamp(false);
    textWriter.setIncludeTimestamp(false);
    csvWriter.setIncludeTimestamp(false);
    
    std::string jsonPath = jsonWriter.writeToFile(testSummary, "specialized_json");
    std::string textPath = textWriter.writeToFile(testSummary, "specialized_text");
    std::string csvPath = csvWriter.writeToFile(testSummary, "specialized_csv");
    
    EXPECT_TRUE(std::filesystem::exists(jsonPath));
    EXPECT_TRUE(std::filesystem::exists(textPath));
    EXPECT_TRUE(std::filesystem::exists(csvPath));
    
    EXPECT_NE(jsonPath.find(".json"), std::string::npos);
    EXPECT_NE(textPath.find(".txt"), std::string::npos);
    EXPECT_NE(csvPath.find(".csv"), std::string::npos);
}

TEST_F(SummaryWriterTest, NumericFieldFormatting) {
    // Add a numeric custom field
    testSummary.addCustomField("pi", "3.141592653589793");
    testSummary.addCustomField("integer", "42");
    
    auto writer = SummaryWriterFactory::create("json", testOutputDir);
    writer->setIncludeTimestamp(false);
    
    std::string outputPath = writer->writeToFile(testSummary, "numeric");
    std::string content = readFileContents(outputPath);
    
    // Numeric fields should be formatted as numbers, not strings
    EXPECT_NE(content.find("\"pi\": 3.141593"), std::string::npos);
    EXPECT_NE(content.find("\"integer\": 42"), std::string::npos);
}