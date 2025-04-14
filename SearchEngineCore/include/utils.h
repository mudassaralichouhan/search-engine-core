#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

namespace utils {
    // Email validation
    [[nodiscard]] bool isValidEmail(std::string_view email);

    // Case-insensitive string comparison
    [[nodiscard]] bool caseInsensitiveCompare(std::string_view str1, std::string_view str2);

    // Load file content
    [[nodiscard]] std::string loadFile(const fs::path& path);

    // Get MIME type based on file extension
    [[nodiscard]] std::string getMimeType(const fs::path& path);

    // Load static file with security checks
    [[nodiscard]] bool loadStaticFile(const fs::path& basePath, std::string_view requestPath,
        std::string& content, std::string& mimeType);
}