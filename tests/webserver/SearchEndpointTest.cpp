#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sstream>
#include <iostream>

#include "../../include/search_api.h"
#include "../../include/api.h"
#include "../../include/Logger.h"

using json = nlohmann::json;

// Global variables for test setup
static std::atomic<int> g_serverPort{0};
static std::thread g_serverThread;
static std::atomic<bool> g_serverRunning{false};
static us_listen_socket_t* g_listenSocket = nullptr;

// Helper function to execute shell command and get output
std::string exec(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Start the uWebSockets server on port 9999
void startTestServer() {
    g_serverPort = 9999;
    
    g_serverThread = std::thread([port = g_serverPort.load()]() {
        // Initialize logger for the server
        Logger::getInstance().init(LogLevel::WARNING, false);
        
        uWS::App()
            .get("/search", [](auto* res, auto* req) {
                search_api::handleSearch(res, req);
            })
            .listen(port, [port](auto* token) {
                if (token) {
                    g_listenSocket = (us_listen_socket_t*)token;
                    LOG_INFO("Test server listening on port " + std::to_string(port));
                    g_serverRunning = true;
                } else {
                    LOG_ERROR("Failed to listen on port " + std::to_string(port));
                }
            })
            .run();
            
        g_serverRunning = false;
    });
    
    // Wait for server to start
    auto start = std::chrono::steady_clock::now();
    while (!g_serverRunning && 
           std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!g_serverRunning) {
        throw std::runtime_error("Failed to start test server");
    }
    
    // Additional wait to ensure server is fully ready
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// Stop the test server
void stopTestServer() {
    if (g_listenSocket) {
        us_listen_socket_close(0, g_listenSocket);
        g_listenSocket = nullptr;
    }
    
    if (g_serverThread.joinable()) {
        g_serverThread.detach();
    }
    g_serverRunning = false;
}

// HTTP Response structure
struct HttpResponse {
    int status;
    std::string body;
    std::map<std::string, std::string> headers;
};

// Parse curl output to extract status, headers, and body
HttpResponse parseCurlOutput(const std::string& output) {
    HttpResponse response;
    std::istringstream stream(output);
    std::string line;
    bool inHeaders = true;
    bool statusParsed = false;
    
    while (std::getline(stream, line)) {
        if (!statusParsed && line.find("HTTP/") == 0) {
            // Parse status line
            size_t statusPos = line.find(' ');
            if (statusPos != std::string::npos) {
                response.status = std::stoi(line.substr(statusPos + 1, 3));
                statusParsed = true;
            }
        } else if (inHeaders && line.find(": ") != std::string::npos) {
            // Parse header
            size_t colonPos = line.find(": ");
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 2);
            // Remove \r if present
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            response.headers[key] = value;
        } else if (inHeaders && (line.empty() || line == "\r")) {
            // Empty line marks end of headers
            inHeaders = false;
        } else if (!inHeaders) {
            // Body content
            response.body += line + "\n";
        }
    }
    
    // Remove trailing newline from body
    if (!response.body.empty() && response.body.back() == '\n') {
        response.body.pop_back();
    }
    
    return response;
}

// Make HTTP GET request using curl
HttpResponse httpGet(const std::string& path) {
    std::string url = "http://localhost:" + std::to_string(g_serverPort) + path;
    std::string cmd = "curl -i -s \"" + url + "\"";
    
    std::string output = exec(cmd);
    return parseCurlOutput(output);
}

TEST_CASE("Search endpoint basic functionality", "[search][webserver]") {
    
    SECTION("Valid search returns 200 with JSON") {
        auto res = httpGet("/search?q=test");
        
        REQUIRE(res.status == 200);
        REQUIRE(res.headers["Content-Type"] == "application/json");
        
        // Parse and validate JSON
        json response = json::parse(res.body);
        
        // Check required fields exist
        REQUIRE(response.contains("meta"));
        REQUIRE(response.contains("results"));
        
        // Check meta structure
        REQUIRE(response["meta"].contains("total"));
        REQUIRE(response["meta"].contains("page"));
        REQUIRE(response["meta"].contains("pageSize"));
        
        // Check results is an array
        REQUIRE(response["results"].is_array());
    }
    
    SECTION("Missing query parameter returns 400") {
        auto res = httpGet("/search");
        
        REQUIRE(res.status == 400);
        REQUIRE(res.headers["Content-Type"] == "application/json");
        
        json response = json::parse(res.body);
        REQUIRE(response.contains("error"));
        REQUIRE(response["error"]["code"] == "INVALID_REQUEST");
        REQUIRE(response["error"]["details"]["q"] == "Query parameter is required");
    }
    
    SECTION("Invalid page parameter returns 400") {
        auto res = httpGet("/search?q=test&page=1001");
        
        REQUIRE(res.status == 400);
        
        json response = json::parse(res.body);
        REQUIRE(response["error"]["details"]["page"] == "Page must be between 1 and 1000");
    }
    
    SECTION("Invalid limit parameter returns 400") {
        auto res = httpGet("/search?q=test&limit=101");
        
        REQUIRE(res.status == 400);
        
        json response = json::parse(res.body);
        REQUIRE(response["error"]["details"]["page"] == "Limit must be between 1 and 100");
    }
    
    SECTION("CORS headers are present") {
        auto res = httpGet("/search?q=test");
        
        REQUIRE(res.headers["Access-Control-Allow-Origin"] == "*");
        REQUIRE(!res.headers["Access-Control-Allow-Methods"].empty());
        REQUIRE(!res.headers["Access-Control-Allow-Headers"].empty());
    }
}

TEST_CASE("Search endpoint pagination", "[search][webserver]") {
    
    SECTION("Default pagination values") {
        auto res = httpGet("/search?q=test");
        json response = json::parse(res.body);
        
        REQUIRE(response["meta"]["page"] == 1);
        REQUIRE(response["meta"]["pageSize"] == 10);
    }
    
    SECTION("Custom pagination values") {
        auto res = httpGet("/search?q=test&page=2&limit=20");
        json response = json::parse(res.body);
        
        REQUIRE(response["meta"]["page"] == 2);
        REQUIRE(response["meta"]["pageSize"] == 20);
    }
}

TEST_CASE("Search endpoint domain filtering", "[search][webserver]") {
    
    SECTION("Single domain filter") {
        auto res = httpGet("/search?q=test&domain_filter=example.com");
        json response = json::parse(res.body);
        
        // Just verify the request succeeds
        REQUIRE(response.contains("results"));
    }
    
    SECTION("Multiple domain filters") {
        auto res = httpGet("/search?q=test&domain_filter=example.com,test.org,demo.net");
        json response = json::parse(res.body);
        
        // Just verify the request succeeds
        REQUIRE(response.contains("results"));
    }
}

TEST_CASE("Search result structure validation", "[search][webserver]") {
    
    SECTION("Result fields validation") {
        auto res = httpGet("/search?q=test");
        json response = json::parse(res.body);
        
        // If we have results, validate their structure
        if (response["meta"]["total"] > 0 && !response["results"].empty()) {
            for (const auto& result : response["results"]) {
                // Check all required fields
                REQUIRE(result.contains("url"));
                REQUIRE(result.contains("title"));
                REQUIRE(result.contains("snippet"));
                REQUIRE(result.contains("score"));
                REQUIRE(result.contains("timestamp"));
                
                // Validate field types
                REQUIRE(result["url"].is_string());
                REQUIRE(result["title"].is_string());
                REQUIRE(result["snippet"].is_string());
                REQUIRE(result["score"].is_number());
                REQUIRE(result["timestamp"].is_string());
                
                // Validate score range
                double score = result["score"].get<double>();
                REQUIRE(score >= 0.0);
                REQUIRE(score <= 1.0);
                
                // Validate timestamp format (basic check)
                std::string timestamp = result["timestamp"].get<std::string>();
                REQUIRE(timestamp.find("T") != std::string::npos);
                REQUIRE(timestamp.find("Z") != std::string::npos);
            }
        }
    }
}

// JSON Schema validation
TEST_CASE("Search response JSON schema validation", "[search][webserver][schema]") {
    
    SECTION("Response matches schema") {
        auto res = httpGet("/search?q=test");
        json response = json::parse(res.body);
        
        // Basic schema validation (without external library)
        // Check top-level structure
        REQUIRE(response.is_object());
        REQUIRE(response.size() == 2); // meta and results
        
        // Validate meta object
        REQUIRE(response["meta"].is_object());
        REQUIRE(response["meta"]["total"].is_number_integer());
        REQUIRE(response["meta"]["page"].is_number_integer());
        REQUIRE(response["meta"]["pageSize"].is_number_integer());
        
        // Validate results array
        REQUIRE(response["results"].is_array());
        
        // Validate constraints
        REQUIRE(response["meta"]["page"] >= 1);
        REQUIRE(response["meta"]["pageSize"] >= 0);
        REQUIRE(response["meta"]["pageSize"] <= 100);
        REQUIRE(response["meta"]["total"] >= 0);
    }
}

// Integration test with seeded data
TEST_CASE("Search with seeded data", "[search][webserver][integration]") {
    
    SECTION("Empty index returns empty results") {
        // Set up test index name
        setenv("SEARCH_INDEX_NAME", "test_search_endpoint_index", 1);
        
        auto res = httpGet("/search?q=example");
        json response = json::parse(res.body);
        
        // Should get empty results (not an error)
        REQUIRE(response["meta"]["total"] >= 0);
        REQUIRE(response["results"].is_array());
    }
}

// Main test runner with setup/teardown
int main(int argc, char* argv[]) {
    // Set up test environment
    setenv("SEARCH_REDIS_URI", "tcp://127.0.0.1:6379", 1);
    setenv("SEARCH_REDIS_POOL_SIZE", "2", 1);
    
    try {
        startTestServer();
    } catch (const std::exception& e) {
        std::cerr << "Failed to start test server: " << e.what() << std::endl;
        return 1;
    }
    
    // Run the tests
    int result = Catch::Session().run(argc, argv);
    
    // Clean up
    stopTestServer();
    
    return result;
} 