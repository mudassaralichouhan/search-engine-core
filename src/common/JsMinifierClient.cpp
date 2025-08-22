#include "JsMinifierClient.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

// Callback function for libcurl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

JsMinifierClient::JsMinifierClient(const std::string& serviceUrl) 
    : serviceUrl(serviceUrl) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

JsMinifierClient::~JsMinifierClient() {
    curl_global_cleanup();
}

bool JsMinifierClient::isServiceAvailable() const {
    try {
        std::string response = makeHttpRequest("GET", "/health");
        return !response.empty() && response.find("\"status\":\"healthy\"") != std::string::npos;
    } catch (...) {
        return false;
    }
}

std::string JsMinifierClient::minify(const std::string& jsCode, const std::string& level) const {
    try {
        // Choose method based on file size (use file upload for large files)
        size_t codeSize = jsCode.length();
        size_t sizeKB = codeSize / 1024;
        
        if (sizeKB > 100) {
            // Use file upload for large files (>100KB)
            return minifyWithFileUpload(jsCode, level);
        } else {
            // Use JSON payload for small files (≤100KB)
            return minifyWithJson(jsCode, level);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in minification: " << e.what() << std::endl;
        return jsCode; // Return original code on error
    }
}

std::string JsMinifierClient::minifyWithJson(const std::string& jsCode, const std::string& level) const {
    // Create JSON payload
    std::ostringstream payload;
    payload << "{";
    payload << "\"code\":\"" << jsonEscape(jsCode) << "\",";
    payload << "\"level\":\"" << level << "\"";
    payload << "}";

    std::string response = makeHttpRequest("POST", "/minify/json", payload.str());
    
    // Parse response to extract minified code
    // Simple JSON extraction (in production, use a proper JSON library)
    size_t codeStart = response.find("\"code\":\"");
    if (codeStart != std::string::npos) {
        codeStart += 8; // Length of "code":"
        size_t codeEnd = response.find("\",", codeStart);
        if (codeEnd == std::string::npos) {
            codeEnd = response.find("\"}", codeStart);
        }
        if (codeEnd != std::string::npos) {
            std::string minifiedCode = response.substr(codeStart, codeEnd - codeStart);
            // Unescape JSON string (basic implementation)
            std::string result;
            for (size_t i = 0; i < minifiedCode.length(); ++i) {
                if (minifiedCode[i] == '\\' && i + 1 < minifiedCode.length()) {
                    char next = minifiedCode[i + 1];
                    if (next == '\"') result += '\"';
                    else if (next == '\\') result += '\\';
                    else if (next == '/') result += '/';
                    else if (next == 'n') result += '\n';
                    else if (next == 'r') result += '\r';
                    else if (next == 't') result += '\t';
                    else {
                        result += minifiedCode[i];
                        result += next;
                    }
                    ++i;
                } else {
                    result += minifiedCode[i];
                }
            }
            return result;
        }
    }
    
    // If parsing failed, check for error
    if (response.find("\"error\"") != std::string::npos) {
        std::cerr << "JS Minifier service error: " << response << std::endl;
        return jsCode; // Return original code on error
    }
    
    return jsCode; // Fallback
}

std::string JsMinifierClient::minifyWithFileUpload(const std::string& jsCode, const std::string& level) const {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string url = serviceUrl + "/minify/upload";
    
    // Create multipart form data
    struct curl_httppost* formpost = nullptr;
    struct curl_httppost* lastptr = nullptr;
    
    // Add file data
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "jsfile",
                 CURLFORM_BUFFER, "script.js",
                 CURLFORM_BUFFERPTR, jsCode.c_str(),
                 CURLFORM_BUFFERLENGTH, jsCode.length(),
                 CURLFORM_END);
    
    // Add level parameter
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "level",
                 CURLFORM_COPYCONTENTS, level.c_str(),
                 CURLFORM_END);
    
    // Set basic options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // 60 seconds timeout for large files
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    // Perform the request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
        std::string error = "CURL error: ";
        error += curl_easy_strerror(res);
        curl_formfree(formpost);
        curl_easy_cleanup(curl);
        throw std::runtime_error(error);
    }

    // Check HTTP response code
    long responseCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    
    // Cleanup
    curl_formfree(formpost);
    curl_easy_cleanup(curl);

    if (responseCode >= 400) {
        throw std::runtime_error("HTTP error " + std::to_string(responseCode) + ": " + readBuffer);
    }

    // Parse response to extract minified code (same as JSON method)
    size_t codeStart = readBuffer.find("\"code\":\"");
    if (codeStart != std::string::npos) {
        codeStart += 8; // Length of "code":"
        size_t codeEnd = readBuffer.find("\",", codeStart);
        if (codeEnd == std::string::npos) {
            codeEnd = readBuffer.find("\"}", codeStart);
        }
        if (codeEnd != std::string::npos) {
            std::string minifiedCode = readBuffer.substr(codeStart, codeEnd - codeStart);
            // Unescape JSON string (basic implementation)
            std::string result;
            for (size_t i = 0; i < minifiedCode.length(); ++i) {
                if (minifiedCode[i] == '\\' && i + 1 < minifiedCode.length()) {
                    char next = minifiedCode[i + 1];
                    if (next == '\"') result += '\"';
                    else if (next == '\\') result += '\\';
                    else if (next == '/') result += '/';
                    else if (next == 'n') result += '\n';
                    else if (next == 'r') result += '\r';
                    else if (next == 't') result += '\t';
                    else {
                        result += minifiedCode[i];
                        result += next;
                    }
                    ++i;
                } else {
                    result += minifiedCode[i];
                }
            }
            return result;
        }
    }
    
    // If parsing failed, check for error
    if (readBuffer.find("\"error\"") != std::string::npos) {
        std::cerr << "JS Minifier service error: " << readBuffer << std::endl;
        return jsCode; // Return original code on error
    }
    
    return jsCode; // Fallback
}

std::vector<JsMinifierClient::MinifyResult> JsMinifierClient::minifyBatch(
    const std::vector<std::pair<std::string, std::string>>& files, 
    const std::string& level) const {
    
    std::vector<MinifyResult> results;
    
    try {
        // Create JSON payload for batch processing
        std::ostringstream payload;
        payload << "{";
        payload << "\"level\":\"" << level << "\",";
        payload << "\"files\":[";
        
        for (size_t i = 0; i < files.size(); ++i) {
            if (i > 0) payload << ",";
            payload << "{";
            payload << "\"name\":\"" << jsonEscape(files[i].first) << "\",";
            payload << "\"code\":\"" << jsonEscape(files[i].second) << "\"";
            payload << "}";
        }
        
        payload << "]}";

        std::string response = makeHttpRequest("POST", "/minify/batch", payload.str());
        
        // For simplicity, return individual results (in production, parse the full JSON response)
        for (const auto& file : files) {
            MinifyResult result;
            result.code = minify(file.second, level);
            result.originalSize = file.second.length();
            result.minifiedSize = result.code.length();
            result.compressionRatio = std::to_string((result.originalSize - result.minifiedSize) * 100.0 / result.originalSize) + "%";
            results.push_back(result);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error in batch minification: " << e.what() << std::endl;
        // Return original codes on error
        for (const auto& file : files) {
            MinifyResult result;
            result.code = file.second;
            result.error = e.what();
            result.originalSize = file.second.length();
            result.minifiedSize = file.second.length();
            result.compressionRatio = "0%";
            results.push_back(result);
        }
    }
    
    return results;
}

std::string JsMinifierClient::getServiceHealth() const {
    return makeHttpRequest("GET", "/health");
}

std::string JsMinifierClient::getServiceConfig() const {
    return makeHttpRequest("GET", "/config");
}

std::string JsMinifierClient::makeHttpRequest(const std::string& method, const std::string& endpoint, 
                                            const std::string& payload) const {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string url = serviceUrl + endpoint;
    
    // Set basic options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30 seconds timeout
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); // 10 seconds connection timeout

    // Set headers for POST requests
    struct curl_slist* headers = nullptr;
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Perform the request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
        std::string error = "CURL error: ";
        error += curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        if (headers) {
            curl_slist_free_all(headers);
        }
        throw std::runtime_error(error);
    }

    // Check HTTP response code
    long responseCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    
    // Cleanup
    curl_easy_cleanup(curl);
    if (headers) {
        curl_slist_free_all(headers);
    }

    if (responseCode >= 400) {
        throw std::runtime_error("HTTP error " + std::to_string(responseCode) + ": " + readBuffer);
    }

    return readBuffer;
}

std::string JsMinifierClient::urlEncode(const std::string& value) const {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string JsMinifierClient::jsonEscape(const std::string& value) const {
    std::ostringstream escaped;
    for (char c : value) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '/':  escaped << "\\/"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (c >= 0 && c <= 0x1f) {
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    escaped << c;
                }
                break;
        }
    }
    return escaped.str();
}
