#pragma once
#include <uwebsockets/App.h>

struct PerSocketData {};

class WebSocketHandler {
public:
    virtual ~WebSocketHandler() = default;
    virtual void registerEndpoint(uWS::App& app) = 0;
}; 