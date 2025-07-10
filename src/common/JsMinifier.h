#pragma once
#include <string>

class JsMinifier {
public:
    // Singleton pattern for global config
    static JsMinifier& getInstance();

    // Enable or disable minification at runtime
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Minify JS code if enabled, else return original
    std::string process(const std::string& jsCode) const;

private:
    JsMinifier();
    bool enabled;
    std::string minify(const std::string& jsCode) const;
}; 