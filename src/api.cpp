#include "../include/api.h"
#include "../include/utils.h"
#include "../include/mongodb.h"
#include <fstream>
#include <sstream>
#include <map>

namespace api {

    std::map<std::string, std::string> parseQueryString(const std::string& queryString) {
        std::map<std::string, std::string> params;
        size_t pos = 0;
        while ((pos = queryString.find('&')) != std::string::npos) {
            std::string key = queryString.substr(0, pos);
            std::string value = queryString.substr(pos + 1, queryString.find('=', pos) - pos - 1);
            params[key] = value;
        }
        if (!queryString.empty()) {
            std::string key = queryString.substr(0, queryString.find('='));
            std::string value = queryString.substr(queryString.find('=') + 1);
            params[key] = value;
        }
        return params;
    }

    void handleEmailSubscribe(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
        std::string buffer;
        res->onData([res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());

            if (last) {
                try {
                    json j = json::parse(buffer);
                    std::string email = j["email"].get<std::string>();

                    if (!utils::isValidEmail(email)) {
                        res->writeStatus("400 Bad Request");
                        res->writeHeader("Content-Type", "application/json");
                        res->end("{\"error\": \"Invalid email address\"}");
                        return;
                    }

                    auto result = mongodb().subscribeEmail(email);

                    if (result.success) {
                        res->writeStatus("200 OK");
                        res->writeHeader("Content-Type", "application/json");
                        res->end("{\"message\": \"" + result.message + "\"}");
                    }
                    else {
                        res->writeStatus("400 Bad Request");
                        res->writeHeader("Content-Type", "application/json");
                        res->end("{\"error\": \"" + result.message + "\"}");
                    }
                }
                catch (const std::exception& e) {
                    res->writeStatus("400 Bad Request");
                    res->writeHeader("Content-Type", "application/json");
                    res->end("{\"error\": \"Invalid JSON data: " + std::string(e.what()) + "\"}");
                }
            }
        });

        res->onAborted([]() {
            std::cout << "Request was aborted" << std::endl;
        });
    }

    void handleStaticFile(uWS::HttpResponse<false>* res, uWS::HttpRequest* req, const std::string& path) {
        std::string content;
        std::string mimeType;

        if (utils::loadStaticFile("public", path, content, mimeType)) {
            res->writeHeader("Content-Type", mimeType);
            res->end(content);
        }
        else {
            res->writeStatus("404 Not Found");
            res->writeHeader("Content-Type", "text/plain");
            res->end("File not found");
        }
    }

    void handleRoot(uWS::HttpResponse<false>* res, uWS::HttpRequest* req, const std::string& indexHtml) {
        res->writeHeader("Content-Type", "text/html; charset=utf-8");
        res->end(indexHtml);
    }
} 