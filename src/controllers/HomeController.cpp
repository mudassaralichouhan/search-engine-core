#include "HomeController.h"
#include "../../include/Logger.h"
#include "../../include/api.h"
#include <filesystem>
#include <nlohmann/json.hpp>

// Deep merge helper: fill missing keys in dst with values from src (recursively for objects)
static void jsonDeepMergeMissing(nlohmann::json &dst, const nlohmann::json &src) {
    if (!dst.is_object() || !src.is_object()) return;
    for (auto it = src.begin(); it != src.end(); ++it) {
        const std::string &key = it.key();
        if (dst.contains(key)) {
            if (dst[key].is_object() && it.value().is_object()) {
                jsonDeepMergeMissing(dst[key], it.value());
            }
        } else {
            dst[key] = it.value();
        }
    }
}

// Format integer with thousands separators (e.g., 1000000 -> 1,000,000)
static std::string formatThousands(long long value) {
    bool isNegative = value < 0;
    unsigned long long v = isNegative ? static_cast<unsigned long long>(-value) : static_cast<unsigned long long>(value);
    std::string digits = std::to_string(v);
    std::string out;
    out.reserve(digits.size() + digits.size() / 3);
    int count = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        out.push_back(digits[static_cast<size_t>(i)]);
        count++;
        if (i > 0 && count % 3 == 0) {
            out.push_back(',');
        }
    }
    std::reverse(out.begin(), out.end());
    if (isNegative) out.insert(out.begin(), '-');
    return out;
}

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
        // Load language metadata (code/dir) from root locale file
        std::string metaData = loadFile("locales/" + defaultLang + ".json");
        if (metaData.empty()) {
            serverError(res, "Failed to load localization metadata");
            return;
        }
        nlohmann::json metaJson = nlohmann::json::parse(metaData);

        // Load sponsor page translations (primary=default for base route)
        std::string sponsorPrimaryStr = loadFile("locales/" + defaultLang + "/sponsor.json");
        std::string sponsorFallbackStr = loadFile("locales/" + getDefaultLocale() + "/sponsor.json");
        nlohmann::json sponsorPrimary = sponsorPrimaryStr.empty() ? nlohmann::json::object() : nlohmann::json::parse(sponsorPrimaryStr);
        nlohmann::json sponsorFallback = sponsorFallbackStr.empty() ? nlohmann::json::object() : nlohmann::json::parse(sponsorFallbackStr);
        jsonDeepMergeMissing(sponsorPrimary, sponsorFallback);

        // Pre-format tier prices with thousands separators
        try {
            if (sponsorPrimary.contains("tiers") && sponsorPrimary["tiers"].is_array()) {
                for (auto &tier : sponsorPrimary["tiers"]) {
                    if (tier.contains("priceUsdYear") && tier["priceUsdYear"].is_number_integer()) {
                        long long py = tier["priceUsdYear"].get<long long>();
                        tier["priceUsdYearFmt"] = formatThousands(py);
                    }
                    if (tier.contains("priceUsdMonth") && (tier["priceUsdMonth"].is_number_integer() || tier["priceUsdMonth"].is_number_float())) {
                        // treat as integer display
                        long long pm = 0;
                        if (tier["priceUsdMonth"].is_number_integer()) pm = tier["priceUsdMonth"].get<long long>();
                        else pm = static_cast<long long>(tier["priceUsdMonth"].get<double>());
                        tier["priceUsdMonthFmt"] = formatThousands(pm);
                    }
                }
            }
        } catch (...) { /* ignore formatting errors */ }

        nlohmann::json t;
        if (metaJson.contains("language")) t["language"] = metaJson["language"];
        t["sponsor"] = sponsorPrimary;

        // Get the host from the request headers
        std::string host = std::string(req->getHeader("host"));
        std::string protocol = "http://";
        
        // Check if we're behind a proxy (X-Forwarded-Proto header)
        std::string forwardedProto = std::string(req->getHeader("x-forwarded-proto"));
        if (!forwardedProto.empty()) {
            protocol = forwardedProto + "://";
        }
        
        std::string baseUrl = protocol + host;

        nlohmann::json templateData = {
            {"t", t},
            {"base_url", baseUrl}
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
        std::string metaFile = "locales/" + langCode + ".json";
        if (!std::filesystem::exists(metaFile)) {
            langCode = getDefaultLocale();
            metaFile = "locales/" + langCode + ".json";
        }
        std::string metaData = loadFile(metaFile);
        if (metaData.empty()) {
            serverError(res, "Failed to load localization metadata for language: " + langCode);
            return;
        }
        nlohmann::json metaJson = nlohmann::json::parse(metaData);

        // Load sponsor translations for requested lang with fallback to default
        std::string sponsorPrimaryStr = loadFile("locales/" + langCode + "/sponsor.json");
        std::string sponsorFallbackStr = loadFile("locales/" + getDefaultLocale() + "/sponsor.json");
        nlohmann::json sponsorPrimary = sponsorPrimaryStr.empty() ? nlohmann::json::object() : nlohmann::json::parse(sponsorPrimaryStr);
        nlohmann::json sponsorFallback = sponsorFallbackStr.empty() ? nlohmann::json::object() : nlohmann::json::parse(sponsorFallbackStr);
        jsonDeepMergeMissing(sponsorPrimary, sponsorFallback);

        // Pre-format tier prices with thousands separators
        try {
            if (sponsorPrimary.contains("tiers") && sponsorPrimary["tiers"].is_array()) {
                for (auto &tier : sponsorPrimary["tiers"]) {
                    if (tier.contains("priceUsdYear") && tier["priceUsdYear"].is_number_integer()) {
                        long long py = tier["priceUsdYear"].get<long long>();
                        tier["priceUsdYearFmt"] = formatThousands(py);
                    }
                    if (tier.contains("priceUsdMonth") && (tier["priceUsdMonth"].is_number_integer() || tier["priceUsdMonth"].is_number_float())) {
                        long long pm = 0;
                        if (tier["priceUsdMonth"].is_number_integer()) pm = tier["priceUsdMonth"].get<long long>();
                        else pm = static_cast<long long>(tier["priceUsdMonth"].get<double>());
                        tier["priceUsdMonthFmt"] = formatThousands(pm);
                    }
                }
            }
        } catch (...) { /* ignore formatting errors */ }

        nlohmann::json t;
        if (metaJson.contains("language")) t["language"] = metaJson["language"];
        t["sponsor"] = sponsorPrimary;

        // Get the host from the request headers
        std::string host = std::string(req->getHeader("host"));
        std::string protocol = "http://";
        
        // Check if we're behind a proxy (X-Forwarded-Proto header)
        std::string forwardedProto = std::string(req->getHeader("x-forwarded-proto"));
        if (!forwardedProto.empty()) {
            protocol = forwardedProto + "://";
        }
        
        std::string baseUrl = protocol + host;

        nlohmann::json templateData = {
            {"t", t},
            {"base_url", baseUrl}
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
        // Load default language metadata
        std::string defaultLang = getDefaultLocale();
        std::string metaStr = loadFile("locales/" + defaultLang + ".json");
        if (metaStr.empty()) { serverError(res, "Failed to load localization metadata"); return; }
        nlohmann::json metaJson = nlohmann::json::parse(metaStr);

        // Load page-specific translations for default lang with fallback to default root (for compatibility)
        std::string pagePrimaryStr = loadFile("locales/" + defaultLang + "/crawl-request.json");
        std::string pageFallbackStr = loadFile("locales/" + defaultLang + ".json");
        nlohmann::json pagePrimary = pagePrimaryStr.empty() ? nlohmann::json::object() : nlohmann::json::parse(pagePrimaryStr);
        nlohmann::json pageFallback = pageFallbackStr.empty() ? nlohmann::json::object() : nlohmann::json::parse(pageFallbackStr);
        jsonDeepMergeMissing(pagePrimary, pageFallback);

        // Compose template data
        nlohmann::json t = pagePrimary;
        if (metaJson.contains("language")) t["language"] = metaJson["language"];
        
        // Get the host from the request headers
        std::string host = std::string(req->getHeader("host"));
        std::string protocol = "http://";
        
        // Check if we're behind a proxy (X-Forwarded-Proto header)
        std::string forwardedProto = std::string(req->getHeader("x-forwarded-proto"));
        if (!forwardedProto.empty()) {
            protocol = forwardedProto + "://";
        }
        
        std::string baseUrl = protocol + host;
        
        nlohmann::json templateData = {
            {"t", t},
            {"base_url", baseUrl}
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
        
        // Check if language meta file exists, fallback to default if not
        std::string metaFile = "locales/" + langCode + ".json";
        if (!std::filesystem::exists(metaFile)) {
            LOG_WARNING("Language file not found: " + metaFile + ", falling back to default");
            langCode = getDefaultLocale();
            metaFile = "locales/" + langCode + ".json";
        }

        // Load language metadata
        std::string metaStr = loadFile(metaFile);
        if (metaStr.empty()) { serverError(res, "Failed to load localization metadata for language: " + langCode); return; }
        nlohmann::json metaJson = nlohmann::json::parse(metaStr);

        // Load page-specific translations with layered fallback: page(lang) <- root(lang) <- page(default) <- root(default)
        std::string pagePrimaryStr = loadFile("locales/" + langCode + "/crawl-request.json");
        std::string rootLangStr    = loadFile("locales/" + langCode + ".json");
        std::string pageDefaultStr = loadFile("locales/" + getDefaultLocale() + "/crawl-request.json");
        std::string rootDefaultStr = loadFile("locales/" + getDefaultLocale() + ".json");
        nlohmann::json j = nlohmann::json::object();
        if (!rootDefaultStr.empty()) j = nlohmann::json::parse(rootDefaultStr);
        if (!pageDefaultStr.empty()) jsonDeepMergeMissing(j, nlohmann::json::parse(pageDefaultStr));
        if (!rootLangStr.empty())    jsonDeepMergeMissing(j, nlohmann::json::parse(rootLangStr));
        if (!pagePrimaryStr.empty()) jsonDeepMergeMissing(j, nlohmann::json::parse(pagePrimaryStr));

        nlohmann::json t = j;
        if (metaJson.contains("language")) t["language"] = metaJson["language"];
        
        // Get the host from the request headers
        std::string host = std::string(req->getHeader("host"));
        std::string protocol = "http://";
        
        // Check if we're behind a proxy (X-Forwarded-Proto header)
        std::string forwardedProto = std::string(req->getHeader("x-forwarded-proto"));
        if (!forwardedProto.empty()) {
            protocol = forwardedProto + "://";
        }
        
        std::string baseUrl = protocol + host;
        
        LOG_INFO("Setting base_url for template: " + baseUrl);
        LOG_INFO("Host header: " + host);
        LOG_INFO("Protocol: " + protocol);
        LOG_INFO("Forwarded proto: " + forwardedProto);
        
        nlohmann::json templateData = {
            {"t", t},
            {"base_url", baseUrl}
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