#include "../../include/routing/RouteRegistry.h"
#include "../../include/Logger.h"

namespace routing {

void RouteRegistry::registerRoute(const Route& route) {
    std::lock_guard<std::mutex> lock(mutex);
    routes.push_back(route);
    LOG_INFO("Registered route: " + methodToString(route.method) + " " + route.path + 
             " -> " + route.controllerName + "::" + route.actionName);
}

} // namespace routing 