#include "StaticFileController.h"
#include "../../include/Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "../common/JsMinifierClient.h"

void StaticFileController::setCSPHeaders(uWS::HttpResponse<false>* res, const std::string& mimeType, std::string& content) {
    // Set basic security headers for all responses
    res->writeHeader("X-Content-Type-Options", "nosniff");
    res->writeHeader("X-Frame-Options", "DENY");
    res->writeHeader("X-XSS-Protection", "1; mode=block");
    res->writeHeader("Referrer-Policy", "strict-origin-when-cross-origin");
    res->writeHeader("Cross-Origin-Opener-Policy", "same-origin");
    res->writeHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains; preload");
}

void StaticFileController::serveStatic(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    std::string path = std::string(req->getUrl());
    
    // Remove query parameters if any
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }
    
    // Construct full file path
    std::string filePath = "public" + path;
    
    // Security: prevent directory traversal
    if (path.find("..") != std::string::npos) {
        notFound(res, "Invalid path");
        return;
    }
    
    // Check if file exists
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
        LOG_WARNING("Static file not found: " + path);
        notFound(res, "File not found");
        return;
    }
    
    // Read file content
    std::string content = readFile(filePath);
    if (content.empty() && std::filesystem::file_size(filePath) > 0) {
        serverError(res, "Failed to read file");
        return;
    }
    
    // Determine MIME type
    std::string mimeType = getMimeType(filePath);
    
    // Minify JS if enabled and file is JS
    if (mimeType == "application/javascript") {
        static JsMinifierClient client("http://js-minifier:3002");
        if (client.isServiceAvailable()) {
            try {
                content = client.minify(content, "advanced");
            } catch (const std::exception& e) {
                LOG_WARNING("JS minification failed, using original content: " + std::string(e.what()));
            }
        }
    }
    
    // Set response headers
    res->writeStatus("200 OK")
       ->writeHeader("Content-Type", mimeType);
    
    // Set basic security headers (no CSP for static files)
    setCSPHeaders(res, mimeType, content);
    
    // Add cache headers for static assets
    if (shouldCache(filePath)) {
        res->writeHeader("Cache-Control", "public, max-age=31536000");
    } else {
        res->writeHeader("Cache-Control", "no-cache");
    }
    
    // Send content
    res->end(content);
    
    LOG_INFO("Served static file: " + path + " (" + std::to_string(content.length()) + " bytes)");
}

std::string StaticFileController::getMimeType(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html; charset=utf-8"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".txt", "text/plain"}
    };
    
    // Get file extension
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos);
        auto it = mimeTypes.find(ext);
        if (it != mimeTypes.end()) {
            return it->second;
        }
    }
    
    return "application/octet-stream";
}

bool StaticFileController::shouldCache(const std::string& path) {
    // Cache images, fonts, and other assets
    static const std::vector<std::string> cacheableExtensions = {
        ".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico",
        ".woff", ".woff2", ".ttf", ".otf",
        ".mp4", ".webm", ".mp3", ".wav"
    };
    
    for (const auto& ext : cacheableExtensions) {
        if (path.ends_with(ext)) {
            return true;
        }
    }
    
    return false;
}

std::string StaticFileController::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: " + path);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
} 