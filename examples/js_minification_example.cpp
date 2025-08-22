#include "../src/common/JsMinifierClient.h"
#include <iostream>
#include <chrono>
#include <vector>

void demonstrateBasicUsage() {
    std::cout << "\n=== JS Minifier Service Usage ===" << std::endl;
    
    JsMinifierClient client("http://localhost:3002");
    
    if (!client.isServiceAvailable()) {
        std::cout << "JS Minifier service is not available. Make sure the service is running." << std::endl;
        return;
    }
    
    std::string jsCode = R"(
// This is a sample JavaScript function
function calculateTotal(items) {
    let total = 0;
    for (let i = 0; i < items.length; i++) {
        total += items[i].price * items[i].quantity;
    }
    console.log('Total calculated:', total);
    return total;
}

/* Another function with debug info */
function debugInfo() {
    console.log('Debug mode enabled');
    debugger;
    return 'debug-info';
}

const myLibrary = {
    version: '1.0.0',
    calculate: calculateTotal,
    debug: debugInfo
};
)";

    std::cout << "\nOriginal code (" << jsCode.length() << " bytes):\n" << jsCode << std::endl;

    // Test different minification levels
    std::vector<std::string> levels = {"basic", "advanced", "aggressive"};

    for (const std::string& level : levels) {
        try {
            auto start = std::chrono::high_resolution_clock::now();
            std::string minified = client.minify(jsCode, level);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            double compressionRatio = (double)(jsCode.length() - minified.length()) / jsCode.length() * 100;
            
            std::cout << "\n--- " << level << " MINIFICATION ---" << std::endl;
            std::cout << "Compressed: " << jsCode.length() << " → " << minified.length() 
                      << " bytes (" << compressionRatio << "% reduction)" << std::endl;
            std::cout << "Processing time: " << duration.count() << "ms" << std::endl;
            std::cout << "Result:\n" << minified << std::endl;
        } catch (const std::exception& e) {
            std::cout << "\n" << level << " minification failed: " << e.what() << std::endl;
        }
    }
}

void demonstrateServiceUsage() {
    std::cout << "\n=== JsMinifier Service Usage ===" << std::endl;
    
    JsMinifierClient client("http://localhost:3002");
    
    if (!client.isServiceAvailable()) {
        std::cout << "JS Minifier service is not available. Make sure the service is running." << std::endl;
        return;
    }
    
    std::cout << "Service is available!" << std::endl;
    std::cout << "Health: " << client.getServiceHealth() << std::endl;
    std::cout << "Config: " << client.getServiceConfig() << std::endl;
    
    std::string jsCode = R"(
function complexFunction(data, options = {}) {
    const { enableLogging = false, maxRetries = 3 } = options;
    
    if (enableLogging) {
        console.log('Processing data:', data);
    }
    
    let result = [];
    let retries = 0;
    
    while (retries < maxRetries) {
        try {
            for (const item of data) {
                if (typeof item === 'object' && item !== null) {
                    result.push({
                        id: item.id || Math.random(),
                        processed: true,
                        timestamp: new Date().toISOString(),
                        data: JSON.stringify(item)
                    });
                }
            }
            break;
        } catch (error) {
            console.error('Processing error:', error);
            retries++;
            if (retries >= maxRetries) {
                throw new Error('Max retries exceeded');
            }
        }
    }
    
    return result;
}
)";

    std::cout << "\nOriginal code (" << jsCode.length() << " bytes):\n" << jsCode << std::endl;

    // Test different levels
    std::vector<std::string> levels = {"basic", "advanced", "aggressive"};
    
    for (const std::string& level : levels) {
        auto start = std::chrono::high_resolution_clock::now();
        std::string minified = client.minify(jsCode, level);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        double compressionRatio = (double)(jsCode.length() - minified.length()) / jsCode.length() * 100;
        
        std::cout << "\n--- " << level << " LEVEL ---" << std::endl;
        std::cout << "Compressed: " << jsCode.length() << " → " << minified.length() 
                  << " bytes (" << compressionRatio << "% reduction)" << std::endl;
        std::cout << "Processing time: " << duration.count() << "ms" << std::endl;
        std::cout << "Result:\n" << minified << std::endl;
    }
}

void demonstrateBatchProcessing() {
    std::cout << "\n=== Batch Processing Example ===" << std::endl;
    
    JsMinifierClient client("http://localhost:3002");
    
    if (!client.isServiceAvailable()) {
        std::cout << "Service not available for batch processing demo." << std::endl;
        return;
    }
    
    // Prepare multiple JavaScript files
    std::vector<std::pair<std::string, std::string>> files = {
        {"utils.js", R"(
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}
)"},
        {"api.js", R"(
class ApiClient {
    constructor(baseUrl) {
        this.baseUrl = baseUrl;
        this.defaultHeaders = {
            'Content-Type': 'application/json',
        };
    }
    
    async get(endpoint) {
        const response = await fetch(`${this.baseUrl}${endpoint}`, {
            headers: this.defaultHeaders
        });
        return response.json();
    }
    
    async post(endpoint, data) {
        const response = await fetch(`${this.baseUrl}${endpoint}`, {
            method: 'POST',
            headers: this.defaultHeaders,
            body: JSON.stringify(data)
        });
        return response.json();
    }
}
)"},
        {"components.js", R"(
const createButton = (text, onClick, className = 'btn') => {
    const button = document.createElement('button');
    button.textContent = text;
    button.className = className;
    button.addEventListener('click', onClick);
    return button;
};

const createModal = (title, content) => {
    const modal = document.createElement('div');
    modal.className = 'modal';
    modal.innerHTML = `
        <div class="modal-content">
            <h2>${title}</h2>
            <div class="modal-body">${content}</div>
            <button class="close-btn">Close</button>
        </div>
    `;
    return modal;
};
)"}
    };
    
    std::cout << "Processing " << files.size() << " JavaScript files..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto results = client.minifyBatch(files, "advanced");
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    size_t totalOriginal = 0;
    size_t totalMinified = 0;
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        totalOriginal += result.originalSize;
        totalMinified += result.minifiedSize;
        
        std::cout << "\nFile: " << files[i].first << std::endl;
        std::cout << "Size: " << result.originalSize << " → " << result.minifiedSize 
                  << " bytes (" << result.compressionRatio << " reduction)" << std::endl;
        if (!result.error.empty()) {
            std::cout << "Error: " << result.error << std::endl;
        }
    }
    
    double overallCompression = (double)(totalOriginal - totalMinified) / totalOriginal * 100;
    std::cout << "\n--- BATCH SUMMARY ---" << std::endl;
    std::cout << "Files processed: " << files.size() << std::endl;
    std::cout << "Total size: " << totalOriginal << " → " << totalMinified 
              << " bytes (" << overallCompression << "% reduction)" << std::endl;
    std::cout << "Total processing time: " << duration.count() << "ms" << std::endl;
}

int main() {
    std::cout << "🗜️  JavaScript Minification Examples" << std::endl;
    std::cout << "====================================" << std::endl;
    
    try {
        // Demonstrate basic minification
        demonstrateBasicUsage();
        
        // Demonstrate service-based minification
        demonstrateServiceUsage();
        
        // Demonstrate batch processing
        demonstrateBatchProcessing();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n✅ All examples completed successfully!" << std::endl;
    return 0;
}
