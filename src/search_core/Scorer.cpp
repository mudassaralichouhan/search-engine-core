#include "search_core/Scorer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace hatef::search {

Scorer::Scorer() {
    setDefaultConfig();
}

Scorer::Scorer(const std::string& config_path) {
    try {
        loadConfig(config_path);
    } catch (const std::exception&) {
        // Fall back to default config if loading fails
        setDefaultConfig();
    }
}

void Scorer::reload(const std::string& config_path) {
    loadConfig(config_path);
}

std::vector<std::string> Scorer::buildArgs() const {
    std::vector<std::string> args;
    
    // Use TFIDF scorer - field weights are already defined in the index
    args.push_back("SCORER");
    args.push_back("TFIDF");
    
    // Note: Field weights should be specified during FT.CREATE, not in FT.SEARCH
    // The WEIGHTS parameter is not a valid argument for FT.SEARCH
    
    return args;
}

void Scorer::loadConfig(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open scoring config file: " + config_path);
    }
    
    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse JSON config: " + std::string(e.what()));
    }
    
    // Parse field weights
    config_.field_weights.clear();
    if (json.contains("field_weights") && json["field_weights"].is_object()) {
        for (auto& [field, weight] : json["field_weights"].items()) {
            if (weight.is_number()) {
                config_.field_weights[field] = weight.get<double>();
            }
        }
    }
    
    // Parse offset boost
    if (json.contains("offset_boost") && json["offset_boost"].is_number()) {
        config_.offset_boost = json["offset_boost"].get<double>();
    }
}

void Scorer::setDefaultConfig() {
    config_.field_weights = {
        {"title", 2.0},
        {"body", 1.0}
    };
    config_.offset_boost = 0.1;
}

} // namespace hatef::search 