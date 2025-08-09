#include "../include/Logger.h"
#include "../include/search_engine/crawler/PageFetcher.h"
#include <iostream>
#include <chrono>

int main() {
    // Initialize logger
    Logger::init();
    
    std::cout << "=== SPA Crawler Example ===" << std::endl;
    
    // Create PageFetcher with SPA rendering enabled
    PageFetcher fetcher(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
        std::chrono::milliseconds(30000),  // 30 second timeout
        true,  // follow redirects
        5      // max redirects
    );
    
    // Enable SPA rendering with browserless
    fetcher.setSpaRendering(true, "http://localhost:3001");  // Use host port for local testing
    
    // Test URLs - some are SPAs, some are not
    std::vector<std::string> testUrls = {
        "https://digikala.com",           // SPA (React-based)
        "https://example.com",            // Static HTML
        "https://httpbin.org/html",       // Static HTML
        "https://vuejs.org",              // SPA (Vue.js)
        "https://reactjs.org"             // SPA (React)
    };
    
    for (const auto& url : testUrls) {
        std::cout << "\n--- Testing URL: " << url << " ---" << std::endl;
        
        auto start = std::chrono::steady_clock::now();
        auto result = fetcher.fetch(url);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Status: " << result.statusCode << std::endl;
        std::cout << "Success: " << (result.success ? "Yes" : "No") << std::endl;
        std::cout << "Content Type: " << result.contentType << std::endl;
        std::cout << "Content Size: " << result.content.size() << " bytes" << std::endl;
        std::cout << "Duration: " << duration.count() << "ms" << std::endl;
        
        if (!result.success) {
            std::cout << "Error: " << result.errorMessage << std::endl;
        } else {
            // Show first 200 characters of content
            std::string preview = result.content.substr(0, 200);
            std::cout << "Content Preview: " << preview << "..." << std::endl;
        }
    }
    
    std::cout << "\n=== Example completed ===" << std::endl;
    return 0;
} 