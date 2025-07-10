#include "HomeController.h"
#include "../../include/Logger.h"
#include "../../include/api.h"
#include <filesystem>

std::string HomeController::loadFile(const std::string& path) {
    LOG_DEBUG("Attempting to load file: " + path);
    
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_ERROR("Error: File does not exist or is not a regular file: " + path);
        return "";
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Error: Could not open file: " + path);
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

void HomeController::index(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("HomeController::index called");
    
    // Load and serve the coming soon page
    static std::string comingSoonHtml = loadFile("public/coming-soon.html");
    
    if (comingSoonHtml.empty()) {
        serverError(res, "Failed to load page");
        return;
    }
    
    html(res, comingSoonHtml);
}

void HomeController::searchPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("HomeController::searchPage called");
    
    // Load and serve the search engine page
    static std::string searchIndexHtml = loadFile("public/index.html");
    
    if (searchIndexHtml.empty()) {
        serverError(res, "Failed to load page");
        return;
    }
    
    html(res, searchIndexHtml);
}

void HomeController::emailSubscribe(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("HomeController::emailSubscribe called");
    
    // Read the request body
    std::string buffer;
    res->onData([this, res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
        buffer.append(data.data(), data.length());
        
        if (last) {
            try {
                // Parse JSON body
                auto jsonBody = nlohmann::json::parse(buffer);
                std::string email = jsonBody.value("email", "");
                
                if (email.empty()) {
                    badRequest(res, "Email is required");
                    return;
                }
                
                // Here you would normally save the email to database
                LOG_INFO("Email subscription received: " + email);
                
                // Return success response
                nlohmann::json response = {
                    {"success", true},
                    {"message", "Successfully subscribed!"}
                };
                
                json(res, response);
                
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse email subscription: " + std::string(e.what()));
                badRequest(res, "Invalid request body");
            }
        }
    });
    
    res->onAborted([]() {
        LOG_WARNING("Email subscription request aborted");
    });
} 