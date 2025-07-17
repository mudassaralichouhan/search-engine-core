#include "websocket/DateTimeWebSocketHandler.h"
#include <chrono>
#include <ctime>

DateTimeWebSocketHandler::DateTimeWebSocketHandler() : running(true) {
    broadcastThread = std::thread([this]() { broadcastLoop(); });
}

DateTimeWebSocketHandler::~DateTimeWebSocketHandler() {
    running = false;
    if (broadcastThread.joinable()) broadcastThread.join();
}

void DateTimeWebSocketHandler::registerEndpoint(uWS::App& app) {
    app.ws<PerSocketData>("/datetime", {
        .open = [this](auto* ws) { clients.insert(ws); },
        .close = [this](auto* ws, int, std::string_view) { clients.erase(ws); }
    });
}

void DateTimeWebSocketHandler::broadcastLoop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string datetime = std::ctime(&now);
        datetime.erase(datetime.find_last_not_of(" \n\r\t")+1);
        for (auto* ws : clients) {
            ws->send(datetime, uWS::OpCode::TEXT);
        }
    }
} 