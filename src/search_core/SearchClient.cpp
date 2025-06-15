#include "search_core/SearchClient.hpp"
#include <sw/redis++/redis++.h>
#include <hiredis/hiredis.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <sstream>
#include <iostream>

namespace hatef::search {

struct SearchClient::Impl {
    std::vector<std::unique_ptr<sw::redis::Redis>> connections;
    std::atomic<std::size_t> cursor{0};
    RedisConfig config;

    explicit Impl(RedisConfig cfg) : config(std::move(cfg)) {
        std::cout << "[SearchClient] Initializing with URI: " << config.uri 
                  << ", pool_size: " << config.pool_size << std::endl;
        
        try {
            connections.reserve(config.pool_size);
            for (std::size_t i = 0; i < config.pool_size; ++i) {
                std::cout << "[SearchClient] Creating connection " << (i+1) 
                          << "/" << config.pool_size << std::endl;
                
                auto redis = std::make_unique<sw::redis::Redis>(config.uri);
                
                // Test the connection by pinging
                std::cout << "[SearchClient] Testing connection with PING..." << std::endl;
                redis->ping();
                std::cout << "[SearchClient] PING successful for connection " << (i+1) << std::endl;
                
                connections.push_back(std::move(redis));
            }
            std::cout << "[SearchClient] Successfully initialized " << connections.size() 
                      << " connections" << std::endl;
        } catch (const sw::redis::Error& e) {
            std::cout << "[SearchClient] Redis error during initialization: " << e.what() << std::endl;
            throw SearchError("Failed to initialize Redis connections: " + std::string(e.what()));
        } catch (const std::exception& e) {
            std::cout << "[SearchClient] Standard exception during initialization: " << e.what() << std::endl;
            throw SearchError("Failed to connect to Redis: " + std::string(e.what()));
        }
    }

    sw::redis::Redis& getConnection() {
        auto idx = cursor.fetch_add(1, std::memory_order_relaxed) % connections.size();
        return *connections[idx];
    }
};

SearchClient::SearchClient(RedisConfig cfg) 
    : p_(std::make_unique<Impl>(std::move(cfg))) {
    std::cout << "[SearchClient] Constructor completed successfully" << std::endl;
}

SearchClient::~SearchClient() = default;

std::string SearchClient::search(std::string_view index,
                                std::string_view query,
                                const std::vector<std::string>& args) {
    std::cout << "[SearchClient::search] Starting search on index: " << index 
              << ", query: " << query << std::endl;
    
    try {
        auto& redis = p_->getConnection();
        std::cout << "[SearchClient::search] Got Redis connection" << std::endl;
        
        // Build the command arguments
        std::vector<std::string> cmd_args;
        cmd_args.reserve(3 + args.size());
        cmd_args.emplace_back("FT.SEARCH");
        cmd_args.emplace_back(index);
        cmd_args.emplace_back(query);
        
        // Add additional arguments
        for (const auto& arg : args) {
            cmd_args.push_back(arg);
        }
        
        std::cout << "[SearchClient::search] Executing FT.SEARCH with " 
                  << cmd_args.size() << " arguments" << std::endl;
        
        // Execute the command and get the result
        auto reply = redis.command(cmd_args.begin(), cmd_args.end());
        
        std::cout << "[SearchClient::search] Command executed successfully" << std::endl;
        
        // Convert raw Redis reply to a simple string representation
        // This is a basic implementation - in production you'd want more robust parsing
        std::ostringstream oss;
        
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            oss << "[";
            for (size_t i = 0; i < reply->elements; ++i) {
                if (i > 0) oss << ",";
                auto element = reply->element[i];
                if (element && element->type == REDIS_REPLY_STRING) {
                    oss << "\"" << std::string(element->str, element->len) << "\"";
                } else if (element && element->type == REDIS_REPLY_INTEGER) {
                    oss << element->integer;
                } else {
                    oss << "null";
                }
            }
            oss << "]";
        } else if (reply && reply->type == REDIS_REPLY_STRING) {
            oss << "\"" << std::string(reply->str, reply->len) << "\"";
        } else if (reply && reply->type == REDIS_REPLY_INTEGER) {
            oss << reply->integer;
        } else {
            oss << "null";
        }
        
        return oss.str();
    } catch (const sw::redis::Error& e) {
        throw SearchError("Search failed: " + std::string(e.what()));
    }
}

} // namespace hatef::search 