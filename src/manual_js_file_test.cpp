#include "src/common/JsMinifierClient.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <iomanip>

int main() {
    std::cout << "🧪 Manual JS File Processing Test" << std::endl;
    std::cout << "=================================" << std::endl;
    
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

    // Create test directory
    const std::string testDir = "/tmp/js_minifier_test";
    if (!std::filesystem::exists(testDir)) {
        std::filesystem::create_directories(testDir);
    }
    
    // Write input file
    const std::string inputFile = testDir + "/input.js";
    std::ofstream input(inputFile);
    input << testJsCode;
    input.close();
    
    std::cout << "📝 Written input file: " << inputFile << std::endl;
    std::cout << "📊 Original size: " << testJsCode.length() << " bytes" << std::endl;
    
    // Test minification service
    JsMinifierClient client("http://localhost:3002");
    
    if (!client.isServiceAvailable()) {
        std::cout << "❌ JS Minifier service is not available!" << std::endl;
        std::cout << "Please start the service with: docker-compose up js-minifier -d" << std::endl;
        return 1;
    }
    
    std::cout << "✅ JS Minifier service is available" << std::endl;
    
    // Test different minification levels
    std::vector<std::string> levels = {"basic", "advanced", "aggressive"};
    
    for (const std::string& level : levels) {
        std::cout << "\n--- " << level << " minification ---" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::string minifiedCode = client.minify(testJsCode, level);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // Write minified code to file
            const std::string outputFile = testDir + "/output." + level + ".js";
            std::ofstream output(outputFile);
            output << minifiedCode;
            output.close();
            
            // Calculate compression ratio
            double compressionRatio = (double)(testJsCode.length() - minifiedCode.length()) / testJsCode.length() * 100;
            
            std::cout << "✅ Minification successful" << std::endl;
            std::cout << "📊 Minified size: " << minifiedCode.length() << " bytes" << std::endl;
            std::cout << "📈 Compression ratio: " << std::fixed << std::setprecision(2) << compressionRatio << "%" << std::endl;
            std::cout << "⏱️  Processing time: " << duration.count() << "ms" << std::endl;
            std::cout << "💾 Output file: " << outputFile << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Minification failed: " << e.what() << std::endl;
        }
    }
    
    std::cout << "\n📁 Generated files:" << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(testDir)) {
        std::cout << "   " << entry.path().filename() << " (" << entry.file_size() << " bytes)" << std::endl;
    }
    
    std::cout << "\n🎉 Manual test completed!" << std::endl;
    return 0;
}
