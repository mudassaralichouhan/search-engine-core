#include "../../include/search_engine/storage/RedisSearchStorage.h"
#include <iostream>

using namespace search_engine::storage;

int main() {
    try {
        RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_stats_index", "doc:");
        
        std::cout << "Testing Redis connection..." << std::endl;
        auto connectionTest = storage.testConnection();
        std::cout << "Connection result: " << (connectionTest.success ? "SUCCESS" : "FAILED") << std::endl;
        if (!connectionTest.success) {
            std::cout << "Error: " << connectionTest.message << std::endl;
            return 1;
        }
        
        std::cout << "\nTesting getDocumentCount..." << std::endl;
        auto countResult = storage.getDocumentCount();
        std::cout << "Document count result: " << (countResult.success ? "SUCCESS" : "FAILED") << std::endl;
        if (countResult.success) {
            std::cout << "Document count: " << countResult.value << std::endl;
        } else {
            std::cout << "Error: " << countResult.message << std::endl;
        }
        
        std::cout << "\nTesting getIndexInfo..." << std::endl;
        auto infoResult = storage.getIndexInfo();
        std::cout << "Index info result: " << (infoResult.success ? "SUCCESS" : "FAILED") << std::endl;
        if (infoResult.success) {
            std::cout << "Index info contains " << infoResult.value.size() << " fields:" << std::endl;
            for (const auto& [key, value] : infoResult.value) {
                std::cout << "  " << key << " = " << value << std::endl;
            }
        } else {
            std::cout << "Error: " << infoResult.message << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 