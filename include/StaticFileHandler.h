#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Static file handler class
class StaticFileHandler {
private:
    const fs::path basePath;
    std::string indexHtml;

public:
    explicit StaticFileHandler(const fs::path& path);
    void handleRequest(auto* res, auto* req);
}; 