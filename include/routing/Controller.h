#pragma once
#include "Route.h"
#include "RouteRegistry.h"
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace routing {

// Base controller class
class Controller {
public:
    Controller() = default;
    virtual ~Controller() = default;

protected:
    // Helper methods for responses
    void json(uWS::HttpResponse<false>* res, const nlohmann::json& data, const std::string& status = "200 OK");
    void text(uWS::HttpResponse<false>* res, const std::string& content, const std::string& status = "200 OK");
    void html(uWS::HttpResponse<false>* res, const std::string& content, const std::string& status = "200 OK");
    void notFound(uWS::HttpResponse<false>* res, const std::string& message = "Not Found");
    void badRequest(uWS::HttpResponse<false>* res, const std::string& message = "Bad Request");
    void serverError(uWS::HttpResponse<false>* res, const std::string& message = "Internal Server Error");

    // Parse query parameters
    std::map<std::string, std::string> parseQuery(uWS::HttpRequest* req);
    
    // Get route parameters (for future implementation with path parameters)
    std::string getParam(uWS::HttpRequest* req, const std::string& name);
};

// Route registration helper
class RouteRegistrar {
public:
    RouteRegistrar(HttpMethod method, const std::string& path, 
                   std::function<void(uWS::HttpResponse<false>*, uWS::HttpRequest*)> handler,
                   const std::string& controllerName, const std::string& actionName) {
        Route route{method, path, handler, controllerName, actionName};
        RouteRegistry::getInstance().registerRoute(route);
    }
};

// Macro for defining controller routes
#define ROUTE_CONTROLLER(ControllerClass) \
    namespace { \
        struct ControllerClass##Routes { \
            ControllerClass##Routes() { \
                registerRoutes(); \
            } \
            void registerRoutes(); \
        }; \
        static ControllerClass##Routes _##ControllerClass##_routes; \
    } \
    void ControllerClass##Routes::registerRoutes()

// Helper macro to register a route
#define REGISTER_ROUTE(method, path, handler, controllerClass) \
    RouteRegistry::getInstance().registerRoute({ \
        method, path, \
        [](uWS::HttpResponse<false>* res, uWS::HttpRequest* req) { \
            static controllerClass controller; \
            controller.handler(res, req); \
        }, \
        #controllerClass, #handler \
    });

} // namespace routing 