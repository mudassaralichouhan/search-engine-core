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
        // queryString = queryString.substr(pos + 1);
    }
    if (!queryString.empty()) {
        string key = queryString.substr(0, queryString.find('='));
        string value = queryString.substr(queryString.find('=') + 1);
        params[key] = value;
    }
    return params;
}




std::string getEnvVarValue(const char* envVarName) {
    char* buf = nullptr;
    size_t sz = 0;

    if (_dupenv_s(&buf, &sz, envVarName) == 0 && buf != nullptr) {
        std::string value(buf);
        free(buf);
        return value;
    }
    else {
        return "";
    }
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
    if (body != nullptr)
        std::fread(body, size, 1, stdin);
    else return nullptr;
    return body;
}
// (int argc, char* argv[], char* envp[])


std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


int main() {
    // Pre-load index.html
    std::string indexHtml = loadFile("public/index.html");


   

    uWS::App()
        // Serve index.html at root
        .get("/", [indexHtml](auto* res, auto* req) {
            api::handleRoot(res, req, indexHtml);
        })
        
        // Handle all static files
        .get("/*", [](auto* res, auto* req) {
            std::string path = std::string(req->getUrl());
            api::handleStaticFile(res, req, path);
        })

        // Handle email subscription endpoint
        .post("/v2/email-subscribe", [](auto* res, auto* req) {
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

