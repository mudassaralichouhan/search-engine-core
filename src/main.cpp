#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS


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


std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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
    
    std::cout << "[" << getCurrentTimestamp() << "] "
              << method << " " << path;
    
    if (!query.empty()) {
        std::cout << "?" << query;
    }
    
    // Log headers
    std::cout << "\nHeaders:";
    // Note: uWebSockets doesn't provide direct header iteration
    // We can log specific headers we're interested in
    std::cout << "\n  User-Agent: " << req->getHeader("user-agent");
    std::cout << "\n  Accept: " << req->getHeader("accept");
    std::cout << "\n  Content-Type: " << req->getHeader("content-type");
    
    std::cout << "\n" << std::endl;
}

int main() {
    // Pre-load index.html
    std::string indexHtml = loadFile("public/index.html");


   

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
        .post("/v2/email-subscribe", [](auto* res, auto* req) {
            traceRequest(res, req);
            api::handleEmailSubscribe(res, req);
        })

        .listen(3000, [](auto* listen_socket) {
            if (listen_socket) {
                std::cout << "Server listening on port 3000" << std::endl;
            }
            else {
                std::cout << "Failed to listen on port 3000" << std::endl;
            }
        })
        .run();

    return 0;
}

