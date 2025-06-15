#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace hatef::search {

struct ScoringConfig {
    std::unordered_map<std::string, double> field_weights;
    double offset_boost = 0.1;
};

class Scorer {
public:
    /// Default constructor with default scoring config
    Scorer();
    
    /// Constructor with custom config file path
    explicit Scorer(const std::string& config_path);
    
    /// Reload scoring configuration from file
    void reload(const std::string& config_path);
    
    /// Build arguments for RedisSearch SCORER parameter
    std::vector<std::string> buildArgs() const;
    
    /// Get current configuration
    const ScoringConfig& getConfig() const { return config_; }

private:
    ScoringConfig config_;
    
    /// Load configuration from JSON file
    void loadConfig(const std::string& config_path);
    
    /// Set default configuration
    void setDefaultConfig();
};

} // namespace hatef::search 