#pragma once
#include <string>
#include <functional>
#include <uwebsockets/App.h>

namespace routing {

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS,
    HEAD
};

struct Route {
    HttpMethod method;
    std::string path;
    std::function<void(uWS::HttpResponse<false>*, uWS::HttpRequest*)> handler;
    std::string controllerName;
    std::string actionName;
};

// Convert HttpMethod enum to string
inline std::string methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::HEAD: return "HEAD";
        default: return "UNKNOWN";
    }
}

} // namespace routing 