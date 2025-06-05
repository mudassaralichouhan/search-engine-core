#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <uwebsockets/App.h>

#include <locale>
#include <codecvt>

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "../include/utils.h"
#include "../include/api.h"
#include "../include/Logger.h"

using namespace std;

#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct User {
    std::string email;
};



// Function to parse query string parameters
map<string, string> parseQueryString(const string& queryString) {
    map<string, string> params;
    size_t pos = 0;
    while ((pos = queryString.find('&')) != string::npos) {
        string key = queryString.substr(0, pos);
        string value = queryString.substr(pos + 1, queryString.find('=', pos) - pos - 1);
        params[key] = value;
    }
    if (!queryString.empty()) {
        string key = queryString.substr(0, queryString.find('='));
        string value = queryString.substr(queryString.find('=') + 1);
        params[key] = value;
    }
    return params;
}




std::string getEnvVarValue(const char* envVarName) {
    const char* value = std::getenv(envVarName);
    return value ? std::string(value) : "";
}





char* get_body()
{
    char* content_length = getenv("CONTENT_LENGTH");
    if (!content_length)
        return nullptr;
    int size = atoi(content_length);
    if (size == 0)
        return nullptr;
    char* body = (char*)calloc(size + 1, sizeof(char));
    if (body != nullptr) {
        size_t read = std::fread(body, size, 1, stdin);
        if (read != 1) {
            free(body);
            return nullptr;
        }
    }
    return body;
}
// (int argc, char* argv[], char* envp[])


// #include <filesystem>
// #include <fstream>
// #include <sstream>
// #include <string>

std::string loadFile(const std::string& path) {
    LOG_DEBUG("Attempting to load file: " + path);
    
    // Check if file exists and user has permission to access it
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_ERROR("Error: File does not exist or is not a regular file: " + path);
        return "";
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Error: Could not open file: " + path);

        LOG_ERROR("Current working directory: " + std::filesystem::current_path().string());
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    if (content.empty()) {
        LOG_WARNING("Warning: File is empty: " + path);
    } else {
        LOG_INFO("Successfully loaded file: " + path + " (size: " + std::to_string(content.length()) + " bytes)");
    }
    
    return content;
}

// Helper function to get current timestamp
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    return ss.str();
}

// Request tracing middleware
void traceRequest(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    std::string_view method = req->getMethod();
    std::string_view path = req->getUrl();
    std::string_view query = req->getQuery();
    
    std::string logMessage = "[" + getCurrentTimestamp() + "] " + std::string(method) + " " + std::string(path);
    
    if (!query.empty()) {
        logMessage += "?" + std::string(query);
    }
    
    // Log headers
    logMessage += "\nHeaders:";
    // Note: uWebSockets doesn't provide direct header iteration
    // We can log specific headers we're interested in
    logMessage += "\n  User-Agent: " + std::string(req->getHeader("user-agent"));
    logMessage += "\n  Accept: " + std::string(req->getHeader("accept"));
    logMessage += "\n  Content-Type: " + std::string(req->getHeader("content-type"));
    
    LOG_INFO(logMessage);
}

int main() {
    // Initialize logger
    Logger::getInstance().init(LogLevel::INFO, true, "server.log");
    
    // Pre-load index.html
    std::string indexHtml = loadFile("public/index.html");

    // Get port from environment variable or use default
    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::stoi(port_env) : 3000;
    LOG_INFO("Using port: " + std::to_string(port));

    uWS::App()
        // Serve index.html at root
        .get("/", [indexHtml](auto* res, auto* req) {
            traceRequest(res, req);
            api::handleRoot(res, req, indexHtml);
        })
        
        // Handle all static files
        .get("/*", [](auto* res, auto* req) {
            traceRequest(res, req);
            std::string path = std::string(req->getUrl());
            api::handleStaticFile(res, req, path);
        })

        // Handle email subscription endpoint
        .post("/api/v2/email-subscribe", [](auto* res, auto* req) {
            traceRequest(res, req);
            api::handleEmailSubscribe(res, req);
        })

        .listen(port, [port](auto* listen_socket) {
            if (listen_socket) {
                LOG_INFO("Server listening on port " + std::to_string(port));
            }
            else {
                LOG_ERROR("Failed to listen on port " + std::to_string(port));
            }
        })
        .run();

    return 0;
}

