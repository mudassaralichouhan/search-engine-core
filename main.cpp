#include <uwebsockets/App.h>
#include <iostream>

int main() {
    uWS::App().get("/*", [](auto *res, auto *req) {
        res->end("Hello from uWebSockets!");
    }).listen(3000, [](auto *token) {
        if (token) {
            std::cout << "Server listening on port 3000" << std::endl;
        }
    }).run();
}
