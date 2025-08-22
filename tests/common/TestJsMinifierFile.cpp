#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "../../src/common/JsMinifierClient.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace std::chrono_literals;

// Helper function to write JavaScript file
void writeJsFile(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    file << content;
    file.close();
}

// Helper function to read file content
std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + filePath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to get file size
size_t getFileSize(const std::string& filePath) {
    return std::filesystem::file_size(filePath);
}

// Helper function to create test directory
void createTestDirectory(const std::string& dirPath) {
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath);
    }
}

// Helper function to clean up test files
void cleanupTestFiles(const std::vector<std::string>& filePaths) {
    for (const auto& path : filePaths) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
        }
    }
}

TEST_CASE("JS Minifier File Processing - Basic Functionality", "[JsMinifier][File]") {
    const std::string testDir = "/tmp/js_minifier_test";
    const std::string inputFile = testDir + "/input.js";
    const std::string outputFile = testDir + "/output.min.js";
    
    // Create test directory
    createTestDirectory(testDir);
    
    // Test JavaScript content
    const std::string testJsCode = R"(
// Sample JavaScript for testing
function calculateTotal(items) {
    let total = 0;
    
    // Calculate subtotal
    for (const item of items) {
        if (item.price && item.quantity) {
            total += item.price * item.quantity;
            console.log('Processing item:', item.name);
        }
    }
    
    // Apply tax
    const taxRate = 0.08;
    const taxAmount = total * taxRate;
    const finalTotal = total + taxAmount;
    
    // Debug information (will be removed in aggressive mode)
    const debugInfo = 'Debug mode enabled';
    debugger;
    
    console.log('Subtotal:', total);
    console.log('Tax:', taxAmount);
    console.log('Final Total:', finalTotal);
    
    return {
        subtotal: total,
        tax: taxAmount,
        total: finalTotal
    };
}

// Unused function (will be removed in aggressive mode)
function unusedFunction() {
    console.log('This function is never called');
    return 'unused';
}

// Export the main function
module.exports = { calculateTotal };
)";

    SECTION("Write JavaScript file and minify it") {
        // Write test JavaScript file
        REQUIRE_NOTHROW(writeJsFile(inputFile, testJsCode));
        REQUIRE(std::filesystem::exists(inputFile));
        
        // Read the file back to verify
        std::string readContent = readFile(inputFile);
        REQUIRE(readContent == testJsCode);
        
        // Test minification service
        JsMinifierClient client("http://localhost:3002");
        
        if (!client.isServiceAvailable()) {
            WARN("JS Minifier service not available, skipping minification tests");
            return;
        }
        
        // Test different minification levels
        std::vector<std::pair<std::string, std::string>> levels = {
            {"basic", "Basic minification"},
            {"advanced", "Advanced minification"},
            {"aggressive", "Aggressive minification"}
        };
        
        for (const auto& [level, description] : levels) {
            DYNAMIC_SECTION(description) {
                // Minify the JavaScript
                std::string minifiedCode;
                REQUIRE_NOTHROW(minifiedCode = client.minify(testJsCode, level));
                
                // Verify minification worked
                REQUIRE_FALSE(minifiedCode.empty());
                REQUIRE(minifiedCode.length() < testJsCode.length());
                
                // Write minified code to output file
                std::string outputPath = outputFile + "." + level;
                REQUIRE_NOTHROW(writeJsFile(outputPath, minifiedCode));
                REQUIRE(std::filesystem::exists(outputPath));
                
                // Verify output file
                std::string savedContent = readFile(outputPath);
                REQUIRE(savedContent == minifiedCode);
                
                // Calculate compression ratio
                double compressionRatio = (double)(testJsCode.length() - minifiedCode.length()) / testJsCode.length() * 100;
                
                std::cout << "\n--- " << description << " ---" << std::endl;
                std::cout << "Original size: " << testJsCode.length() << " bytes" << std::endl;
                std::cout << "Minified size: " << minifiedCode.length() << " bytes" << std::endl;
                std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) << compressionRatio << "%" << std::endl;
                std::cout << "Output file: " << outputPath << std::endl;
                
                // Verify compression is reasonable
                REQUIRE(compressionRatio > 20.0); // At least 20% compression
                REQUIRE(compressionRatio < 95.0); // Not more than 95% (would be suspicious)
            }
        }
    }
    
    // Cleanup
    cleanupTestFiles({inputFile, outputFile + ".basic", outputFile + ".advanced", outputFile + ".aggressive"});
}

TEST_CASE("JS Minifier File Processing - Large File Test", "[JsMinifier][File][Large]") {
    const std::string testDir = "/tmp/js_minifier_test";
    const std::string inputFile = testDir + "/large_input.js";
    const std::string outputFile = testDir + "/large_output.min.js";
    
    createTestDirectory(testDir);
    
    // Generate a larger JavaScript file
    std::ostringstream largeJsCode;
    largeJsCode << "// Large JavaScript file for testing\n";
    largeJsCode << "const largeData = {\n";
    
    // Add many properties to make the file larger
    for (int i = 0; i < 1000; ++i) {
        largeJsCode << "    property" << i << ": 'value" << i << "',\n";
    }
    
    largeJsCode << "};\n\n";
    largeJsCode << "function processLargeData(data) {\n";
    largeJsCode << "    const result = [];\n";
    largeJsCode << "    for (const key in data) {\n";
    largeJsCode << "        if (data.hasOwnProperty(key)) {\n";
    largeJsCode << "            result.push({\n";
    largeJsCode << "                key: key,\n";
    largeJsCode << "                value: data[key],\n";
    largeJsCode << "                processed: true\n";
    largeJsCode << "            });\n";
    largeJsCode << "        }\n";
    largeJsCode << "    }\n";
    largeJsCode << "    return result;\n";
    largeJsCode << "}\n\n";
    largeJsCode << "// Export functions\n";
    largeJsCode << "module.exports = { largeData, processLargeData };\n";
    
    const std::string largeTestCode = largeJsCode.str();
    
    SECTION("Process large JavaScript file") {
        // Write large test file
        REQUIRE_NOTHROW(writeJsFile(inputFile, largeTestCode));
        REQUIRE(std::filesystem::exists(inputFile));
        
        size_t originalSize = getFileSize(inputFile);
        REQUIRE(originalSize > 50000); // Should be at least 50KB
        
        // Test minification service
        JsMinifierClient client("http://localhost:3002");
        
        if (!client.isServiceAvailable()) {
            WARN("JS Minifier service not available, skipping large file test");
            return;
        }
        
        // Measure processing time
        auto start = std::chrono::high_resolution_clock::now();
        
        std::string minifiedCode;
        REQUIRE_NOTHROW(minifiedCode = client.minify(largeTestCode, "advanced"));
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Verify minification
        REQUIRE_FALSE(minifiedCode.empty());
        REQUIRE(minifiedCode.length() < largeTestCode.length());
        
        // Write minified code
        REQUIRE_NOTHROW(writeJsFile(outputFile, minifiedCode));
        REQUIRE(std::filesystem::exists(outputFile));
        
        size_t minifiedSize = getFileSize(outputFile);
        double compressionRatio = (double)(originalSize - minifiedSize) / originalSize * 100;
        
        std::cout << "\n--- Large File Test ---" << std::endl;
        std::cout << "Original size: " << originalSize << " bytes" << std::endl;
        std::cout << "Minified size: " << minifiedSize << " bytes" << std::endl;
        std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) << compressionRatio << "%" << std::endl;
        std::cout << "Processing time: " << duration.count() << "ms" << std::endl;
        
        // Performance requirements
        REQUIRE(duration.count() < 5000); // Should complete within 5 seconds
        REQUIRE(compressionRatio > 30.0); // At least 30% compression for large files
    }
    
    // Cleanup
    cleanupTestFiles({inputFile, outputFile});
}

TEST_CASE("JS Minifier File Processing - Error Handling", "[JsMinifier][File][Error]") {
    const std::string testDir = "/tmp/js_minifier_test";
    const std::string inputFile = testDir + "/error_test.js";
    
    createTestDirectory(testDir);
    
    SECTION("Handle invalid JavaScript syntax") {
        const std::string invalidJs = R"(
// Invalid JavaScript with syntax errors
function brokenFunction() {
    var x = 1;
    var y = 2;
    if (x == y {  // Missing closing parenthesis
        console.log("This should cause an error");
    }
    return x + y;
}
)";
        
        // Write invalid JavaScript file
        REQUIRE_NOTHROW(writeJsFile(inputFile, invalidJs));
        REQUIRE(std::filesystem::exists(inputFile));
        
        JsMinifierClient client("http://localhost:3002");
        
        if (!client.isServiceAvailable()) {
            WARN("JS Minifier service not available, skipping error handling test");
            return;
        }
        
        // The service should handle invalid JavaScript gracefully
        std::string result;
        REQUIRE_NOTHROW(result = client.minify(invalidJs, "basic"));
        
        // Should return original code or handle error gracefully
        REQUIRE_FALSE(result.empty());
    }
    
    SECTION("Handle empty file") {
        const std::string emptyJs = "";
        
        REQUIRE_NOTHROW(writeJsFile(inputFile, emptyJs));
        
        JsMinifierClient client("http://localhost:3002");
        
        if (!client.isServiceAvailable()) {
            WARN("JS Minifier service not available, skipping empty file test");
            return;
        }
        
        std::string result;
        REQUIRE_NOTHROW(result = client.minify(emptyJs, "basic"));
        
        // Should handle empty input gracefully
        REQUIRE((result.empty() || result == emptyJs));
    }
    
    // Cleanup
    cleanupTestFiles({inputFile});
}

TEST_CASE("JS Minifier File Processing - Batch Processing", "[JsMinifier][File][Batch]") {
    const std::string testDir = "/tmp/js_minifier_test";
    createTestDirectory(testDir);
    
    // Create multiple test files
    std::vector<std::string> inputFiles;
    std::vector<std::string> outputFiles;
    std::vector<std::pair<std::string, std::string>> testFiles;
    
    for (int i = 1; i <= 3; ++i) {
        std::string inputFile = testDir + "/test" + std::to_string(i) + ".js";
        std::string outputFile = testDir + "/test" + std::to_string(i) + ".min.js";
        
        inputFiles.push_back(inputFile);
        outputFiles.push_back(outputFile);
        
        std::string jsCode = R"(
// Test file )" + std::to_string(i) + R"(
function testFunction)" + std::to_string(i) + R"() {
    const data = "test data )" + std::to_string(i) + R"(";
    console.log('Processing:', data);
    return data.toUpperCase();
}

module.exports = { testFunction)" + std::to_string(i) + R"( };
)";
        
        testFiles.push_back({inputFile, jsCode});
    }
    
    SECTION("Process multiple files in batch") {
        // Write all test files
        for (const auto& [filePath, content] : testFiles) {
            REQUIRE_NOTHROW(writeJsFile(filePath, content));
            REQUIRE(std::filesystem::exists(filePath));
        }
        
        JsMinifierClient client("http://localhost:3002");
        
        if (!client.isServiceAvailable()) {
            WARN("JS Minifier service not available, skipping batch processing test");
            return;
        }
        
        // Process each file individually
        for (size_t i = 0; i < testFiles.size(); ++i) {
            const auto& [inputPath, originalContent] = testFiles[i];
            const std::string& outputPath = outputFiles[i];
            
            std::string minifiedCode;
            REQUIRE_NOTHROW(minifiedCode = client.minify(originalContent, "advanced"));
            
            REQUIRE_FALSE(minifiedCode.empty());
            REQUIRE(minifiedCode.length() < originalContent.length());
            
            // Write minified code
            REQUIRE_NOTHROW(writeJsFile(outputPath, minifiedCode));
            REQUIRE(std::filesystem::exists(outputPath));
            
            // Verify file sizes
            size_t originalSize = getFileSize(inputPath);
            size_t minifiedSize = getFileSize(outputPath);
            
            REQUIRE(minifiedSize < originalSize);
            
            std::cout << "\n--- Batch File " << (i + 1) << " ---" << std::endl;
            std::cout << "Original: " << originalSize << " bytes" << std::endl;
            std::cout << "Minified: " << minifiedSize << " bytes" << std::endl;
            std::cout << "Compression: " << std::fixed << std::setprecision(2) 
                      << (double)(originalSize - minifiedSize) / originalSize * 100 << "%" << std::endl;
        }
    }
    
    // Cleanup
    cleanupTestFiles(inputFiles);
    cleanupTestFiles(outputFiles);
}

int main(int argc, char* argv[]) {
    // Set up test environment
    std::cout << "JS Minifier File Processing Tests" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Check if service is available before running tests
    JsMinifierClient client("http://localhost:3002");
    if (!client.isServiceAvailable()) {
        std::cout << "WARNING: JS Minifier service is not available!" << std::endl;
        std::cout << "Make sure the service is running on http://localhost:3002" << std::endl;
        std::cout << "Some tests will be skipped." << std::endl;
    } else {
        std::cout << "JS Minifier service is available." << std::endl;
    }
    
    std::cout << std::endl;
    
    // Run Catch2 tests
    return Catch::Session().run(argc, argv);
}
