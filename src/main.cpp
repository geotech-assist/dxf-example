#include "DXFReader.h"
#include "MeshSummarizer.h"
#include "SummaryWriter.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <memory>
#include <string>

using namespace DXFProcessor;

void printUsage(const char* programName) {
    std::cout << "DXF Processor - Cross-platform DXF mesh analyzer\n\n";
    std::cout << "Usage: " << programName << " [options] <dxf_file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o, --output <dir>     Output directory (default: current directory)\n";
    std::cout << "  -f, --format <format>  Output format: json, text, csv (default: json)\n";
    std::cout << "  -s, --summarizer <type> Summarizer type: basic, detailed (default: basic)\n";
    std::cout << "  -n, --name <basename>  Output file base name (default: mesh_summary)\n";
    std::cout << "  --no-timestamp         Don't include timestamp in filename\n";
    std::cout << "  --no-pretty            Compact JSON output (if using JSON format)\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "  -v, --version          Show version information\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << programName << " --format json --output ./results data/mesh.dxf\n";
}

void printVersion() {
    std::cout << "DXF Processor v1.0.0\n";
    std::cout << "Built with C++17 for cross-platform compatibility\n";
    std::cout << "Supports Windows (Visual Studio), Linux (GCC), macOS (Clang)\n";
}

void showProgress(double progress) {
    static int lastPercent = -1;
    int percent = static_cast<int>(progress * 100);
    
    if (percent != lastPercent && percent % 10 == 0) {
        std::cout << "Processing: " << percent << "% complete\r" << std::flush;
        lastPercent = percent;
    }
    
    if (progress >= 1.0) {
        std::cout << "Processing: 100% complete\n";
    }
}

struct CommandLineArgs {
    std::string inputFile;
    std::string outputDir = ".";
    std::string outputFormat = "json";
    std::string summarizerType = "basic";
    std::string baseName = "mesh_summary";
    bool includeTimestamp = true;
    bool prettyPrint = true;
    bool showHelp = false;
    bool showVersion = false;
};

CommandLineArgs parseCommandLine(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            args.showHelp = true;
        } else if (arg == "-v" || arg == "--version") {
            args.showVersion = true;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            args.outputDir = argv[++i];
        } else if ((arg == "-f" || arg == "--format") && i + 1 < argc) {
            args.outputFormat = argv[++i];
        } else if ((arg == "-s" || arg == "--summarizer") && i + 1 < argc) {
            args.summarizerType = argv[++i];
        } else if ((arg == "-n" || arg == "--name") && i + 1 < argc) {
            args.baseName = argv[++i];
        } else if (arg == "--no-timestamp") {
            args.includeTimestamp = false;
        } else if (arg == "--no-pretty") {
            args.prettyPrint = false;
        } else if (arg[0] != '-') {
            args.inputFile = arg;
        }
    }
    
    return args;
}

int main(int argc, char* argv[]) {
    try {
        CommandLineArgs args = parseCommandLine(argc, argv);
        
        if (args.showHelp) {
            printUsage(argv[0]);
            return 0;
        }
        
        if (args.showVersion) {
            printVersion();
            return 0;
        }
        
        if (args.inputFile.empty()) {
            std::cerr << "Error: No input file specified.\n";
            printUsage(argv[0]);
            return 1;
        }
        
        if (!std::filesystem::exists(args.inputFile)) {
            std::cerr << "Error: Input file does not exist: " << args.inputFile << "\n";
            return 1;
        }
        
        std::cout << "DXF Processor v1.0.0\n";
        std::cout << "Processing: " << args.inputFile << "\n";
        std::cout << "Output directory: " << std::filesystem::absolute(args.outputDir) << "\n";
        std::cout << "Output format: " << args.outputFormat << "\n";
        std::cout << "Summarizer: " << args.summarizerType << "\n\n";
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        auto reader = DXFReaderFactory::createReader();
        reader->setProgressCallback(showProgress);
        
        std::cout << "Reading DXF file...\n";
        auto meshData = reader->readFile(args.inputFile);
        
        std::cout << "Read " << meshData->getTriangleCount() << " triangles from DXF file.\n";
        
        std::cout << "Analyzing mesh...\n";
        auto summarizer = MeshSummarizerFactory::create(args.summarizerType);
        auto summary = summarizer->summarize(*meshData);
        
        std::cout << "Writing summary...\n";
        auto writer = SummaryWriterFactory::create(args.outputFormat, args.outputDir);
        writer->setIncludeTimestamp(args.includeTimestamp);
        writer->setPrettyPrint(args.prettyPrint);
        
        std::string outputPath = writer->writeToFile(summary, args.baseName);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "\nProcessing completed successfully!\n";
        std::cout << "Output written to: " << outputPath << "\n";
        std::cout << "Processing time: " << duration.count() << " ms\n";
        
        std::cout << "\nSummary:\n";
        std::cout << "  Triangles: " << summary.triangleCount << "\n";
        std::cout << "  Surface Area: " << std::fixed << std::setprecision(2) 
                  << summary.totalSurfaceArea << "\n";
        std::cout << "  Bounding Box: (" 
                  << summary.boundingBox.min.x << ", " << summary.boundingBox.min.y << ", " << summary.boundingBox.min.z
                  << ") to ("
                  << summary.boundingBox.max.x << ", " << summary.boundingBox.max.y << ", " << summary.boundingBox.max.z
                  << ")\n";
        
        auto size = summary.boundingBox.size();
        std::cout << "  Dimensions: " << size.x << " x " << size.y << " x " << size.z << "\n";
        
        return 0;
        
    } catch (const DXFReaderException& e) {
        std::cerr << "DXF Reader Error: " << e.what() << "\n";
        return 2;
    } catch (const SummaryWriterException& e) {
        std::cerr << "Summary Writer Error: " << e.what() << "\n";
        return 3;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred.\n";
        return 1;
    }
}