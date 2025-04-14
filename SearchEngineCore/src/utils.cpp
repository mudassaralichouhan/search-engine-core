#include "../include/utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace utils {
    // Email validation
    [[nodiscard]] bool isValidEmail(std::string_view email) {
        return email.find('@') != std::string_view::npos;
    }

    // Case-insensitive string comparison
    [[nodiscard]] bool caseInsensitiveCompare(std::string_view str1, std::string_view str2) {
        if (str1.size() != str2.size()) return false;
        
        return std::equal(str1.begin(), str1.end(), str2.begin(), 
            [](unsigned char a, unsigned char b) {
                return std::tolower(a) == std::tolower(b);
            });
    }

    // Load file content
    [[nodiscard]] std::string loadFile(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + path.string());
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Get MIME type based on file extension
    [[nodiscard]] std::string getMimeType(const fs::path& path) {
        static const std::map<std::string, std::string> mimeTypes = {
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".json", "application/json"},
            {".png", "image/png"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".gif", "image/gif"},
            {".svg", "image/svg+xml"},
            {".ico", "image/x-icon"}
        };

        const auto ext = path.extension().string();
        const auto it = mimeTypes.find(ext);
        return it != mimeTypes.end() ? it->second : "application/octet-stream";
    }

    // Load static file with security checks
    [[nodiscard]] bool loadStaticFile(const fs::path& basePath, std::string_view requestPath, 
                                     std::string& content, std::string& mimeType) {
        // Remove leading slash and sanitize path
        std::string relativePath = requestPath.data();
        if (!relativePath.empty() && relativePath[0] == '/') {
            relativePath = relativePath.substr(1);
        }

        // Construct full file path
        fs::path fullPath = basePath / relativePath;

        // Basic security check to prevent directory traversal
        if (relativePath.find("..") != std::string::npos) {
            return false;
        }

        // Check if file exists and is a regular file
        if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
            return false;
        }

        try {
            content = loadFile(fullPath);
            mimeType = getMimeType(fullPath);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
} 