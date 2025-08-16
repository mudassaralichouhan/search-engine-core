#include "HomeController.h"
#include "../../include/Logger.h"
#include "../../include/api.h"
#include <filesystem>
#include <nlohmann/json.hpp>

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

std::string HomeController::renderTemplate(const std::string& templateName, const nlohmann::json& data) {
    try {
        // Initialize Inja environment
        inja::Environment env("templates/");
        
        // Load the template and render with data  
        std::string result = env.render_file(templateName, data);
        return result;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Template rendering error: " + std::string(e.what()));
        return "";
    }
}

void HomeController::sponsorPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("HomeController::sponsorPage called");
    try {
        std::string defaultLang = getDefaultLocale();
        std::string localeData = loadFile("locales/" + defaultLang + ".json");
        if (localeData.empty()) {
            serverError(res, "Failed to load localization data");
            return;
        }
        nlohmann::json localeJson = nlohmann::json::parse(localeData);
        nlohmann::json templateData = {
            {"t", localeJson},
            {"base_url", "http://localhost:3000"}
        };
        std::string renderedHtml = renderTemplate("sponsor.inja", templateData);
        if (renderedHtml.empty()) {
            serverError(res, "Failed to render sponsor template");
            return;
        }
        html(res, renderedHtml);
    } catch (const std::exception& e) {
        LOG_ERROR("Error in sponsorPage: " + std::string(e.what()));
        serverError(res, "Failed to load sponsor page");
    }
}

void HomeController::sponsorPageWithLang(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("HomeController::sponsorPageWithLang called");
    try {
        std::string url = std::string(req->getUrl());
        size_t lastSlash = url.find_last_of('/');
        std::string langCode;
        if (lastSlash != std::string::npos && lastSlash < url.length() - 1) {
            langCode = url.substr(lastSlash + 1);
        }
        std::string localeFile = "locales/" + langCode + ".json";
        if (!std::filesystem::exists(localeFile)) {
            langCode = getDefaultLocale();
            localeFile = "locales/" + langCode + ".json";
        }
        std::string localeData = loadFile(localeFile);
        if (localeData.empty()) {
            serverError(res, "Failed to load localization data for language: " + langCode);
            return;
        }
        nlohmann::json localeJson = nlohmann::json::parse(localeData);
        nlohmann::json templateData = {
            {"t", localeJson},
            {"base_url", "http://localhost:3000"}
        };
        std::string renderedHtml = renderTemplate("sponsor.inja", templateData);
        if (renderedHtml.empty()) {
            serverError(res, "Failed to render sponsor template");
            return;
        }
        html(res, renderedHtml);
    } catch (const std::exception& e) {
        LOG_ERROR("Error in sponsorPageWithLang: " + std::string(e.what()));
        serverError(res, "Failed to load sponsor page with language");
    }
}

void HomeController::crawlRequestPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("HomeController::crawlRequestPage called");
    
    try {
        // Load default localization data (English)
        std::string defaultLang = getDefaultLocale();
        std::string localeData = loadFile("locales/" + defaultLang + ".json");
        if (localeData.empty()) {
            serverError(res, "Failed to load localization data");
            return;
        }
        
        // Parse JSON data
        nlohmann::json localeJson = nlohmann::json::parse(localeData);
        nlohmann::json templateData = {
            {"t", localeJson},
            {"base_url", "http://localhost:3000"}
        };
        
        // Render template with data
        std::string renderedHtml = renderTemplate("crawl-request-full.inja", templateData);
        
        if (renderedHtml.empty()) {
            serverError(res, "Failed to render template");
            return;
        }
        
        html(res, renderedHtml);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error in crawlRequestPage: " + std::string(e.what()));
        serverError(res, "Failed to load crawl request page");
    }
}

void HomeController::crawlRequestPageWithLang(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("HomeController::crawlRequestPageWithLang called");
    
    try {
        // Extract language code from URL path
        std::string url = std::string(req->getUrl());
        LOG_INFO("Full URL: " + url);
        
        // Extract language code from /crawl-request/lang format
        std::string langCode;
        size_t lastSlash = url.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash < url.length() - 1) {
            langCode = url.substr(lastSlash + 1);
        }
        
        LOG_INFO("Extracted language code: " + langCode);
        
        // Check if language file exists, fallback to default if not
        std::string localeFile = "locales/" + langCode + ".json";
        if (!std::filesystem::exists(localeFile)) {
            LOG_WARNING("Language file not found: " + localeFile + ", falling back to default");
            langCode = getDefaultLocale();
            localeFile = "locales/" + langCode + ".json";
        }
        
        // Load localization data
        std::string localeData = loadFile(localeFile);
        if (localeData.empty()) {
            serverError(res, "Failed to load localization data for language: " + langCode);
            return;
        }
        
        // Parse JSON data
        nlohmann::json localeJson = nlohmann::json::parse(localeData);
        nlohmann::json templateData = {
            {"t", localeJson},
            {"base_url", "http://localhost:3000"}
        };
        
        // Render template with data
        std::string renderedHtml = renderTemplate("crawl-request-full.inja", templateData);
        
        if (renderedHtml.empty()) {
            serverError(res, "Failed to render template");
            return;
        }
        
        html(res, renderedHtml);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error in crawlRequestPageWithLang: " + std::string(e.what()));
        serverError(res, "Failed to load crawl request page with language");
    }
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

std::string HomeController::getAvailableLocales() {
    try {
        std::vector<std::string> locales;
        
        // Scan the locales directory for JSON files
        for (const auto& entry : std::filesystem::directory_iterator("locales/")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().stem().string();
                // Skip test files or other non-locale files
                if (filename != "test-data") {
                    locales.push_back(filename);
                }
            }
        }
        
        // Convert to JSON array string for client-side usage if needed
        nlohmann::json localeArray = locales;
        return localeArray.dump();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting available locales: " + std::string(e.what()));
        return "[\"en\"]"; // Fallback to English only
    }
}

std::string HomeController::getDefaultLocale() {
    return "en"; // English as default
} 