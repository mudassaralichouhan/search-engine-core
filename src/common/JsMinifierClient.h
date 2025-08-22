#pragma once
#include <string>
#include <memory>
#include <vector>

/**
 * HTTP client for JS Minifier microservice
 * Alternative to subprocess approach for better scalability
 */
class JsMinifierClient {
public:
    JsMinifierClient(const std::string& serviceUrl = "http://js-minifier:3002");
    ~JsMinifierClient();

    // Check if the service is available
    bool isServiceAvailable() const;

    // Minify JavaScript code using the microservice
    // Automatically chooses between JSON payload (≤100KB) and file upload (>100KB)
    std::string minify(const std::string& jsCode, const std::string& level = "advanced") const;

    // Batch minify multiple JavaScript files
    struct MinifyResult {
        std::string code;
        std::string error;
        size_t originalSize;
        size_t minifiedSize;
        std::string compressionRatio;
    };
    
    std::vector<MinifyResult> minifyBatch(const std::vector<std::pair<std::string, std::string>>& files, 
                                         const std::string& level = "advanced") const;

    // Get service health and configuration
    std::string getServiceHealth() const;
    std::string getServiceConfig() const;

private:
    std::string serviceUrl;
    
    // Helper methods for HTTP communication
    std::string makeHttpRequest(const std::string& method, const std::string& endpoint, 
                               const std::string& payload = "") const;
    std::string urlEncode(const std::string& value) const;
    std::string jsonEscape(const std::string& value) const;
    
    // Minification methods for different approaches
    std::string minifyWithJson(const std::string& jsCode, const std::string& level) const;
    std::string minifyWithFileUpload(const std::string& jsCode, const std::string& level) const;
};
