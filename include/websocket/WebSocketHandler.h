#pragma once
#include <uwebsockets/App.h>
#include <string>

struct PerSocketData {
    bool open = false;
    std::string userTopic; // For pub/sub pattern
    std::string sessionId; // For session-specific logs
    bool isAdmin = false; // For admin vs user access
};

class WebSocketHandler {
protected:
    uWS::App* app = nullptr; // Reference to the app for pub/sub
    
public:
    virtual ~WebSocketHandler() = default;
    virtual void registerEndpoint(uWS::App& app) = 0;
}; 