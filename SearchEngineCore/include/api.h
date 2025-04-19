#pragma once

#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <string>
#include <map>

using json = nlohmann::json;

namespace api {
    // Function to parse query string parameters
    std::map<std::string, std::string> parseQueryString(const std::string& queryString);

    // Function to handle email subscription
    void handleEmailSubscribe(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);

    // Function to handle static file serving
    void handleStaticFile(uWS::HttpResponse<false>* res, uWS::HttpRequest* req, const std::string& path);

    // Function to handle root route
    void handleRoot(uWS::HttpResponse<false>* res, uWS::HttpRequest* req, const std::string& indexHtml);
} 