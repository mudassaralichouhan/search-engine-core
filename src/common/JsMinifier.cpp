#include "JsMinifier.h"
#include <regex>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <functional>
#include <vector>
#include <cctype> // For isalpha and isalnum
#include <algorithm> // For std::find

JsMinifier::JsMinifier() {
    // Read from env or default to true
    const char* env = std::getenv("MINIFY_JS");
    enabled = (env && std::string(env) == "true");
}

JsMinifier& JsMinifier::getInstance() {
    static JsMinifier instance;
    return instance;
}

void JsMinifier::setEnabled(bool e) { enabled = e; }
bool JsMinifier::isEnabled() const { return enabled; }

std::string JsMinifier::process(const std::string& jsCode) const {
    if (!enabled) return jsCode;
    return minify(jsCode);
}

// Helper function to check if character is alphanumeric
static bool is_alphanum(int c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || 
           (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' || c > 126;
}

std::string JsMinifier::minify(const std::string& jsCode) const {
    std::string result;
    size_t i = 0;
    bool in_string = false;
    bool in_regex = false;
    char string_delimiter = 0;
    bool last_was_space = false;
    bool need_space = false;
    std::string last_token;
    const std::vector<std::string> keywords = {
        // ES5/ES6/ESNext keywords and reserved words
        "break", "case", "catch", "class", "const", "continue", "debugger", "default", "delete", "do", "else", "export", "extends", "finally", "for", "function", "if", "import", "in", "instanceof", "let", "new", "return", "super", "switch", "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",
        // Future reserved words
        "enum", "await", "implements", "package", "protected", "static", "interface", "private", "public",
        // Literals (for completeness)
        "null", "true", "false"
    };
    
    auto is_keyword = [&](const std::string& token) {
        return std::find(keywords.begin(), keywords.end(), token) != keywords.end();
    };
    
    while (i < jsCode.length()) {
        char c = jsCode[i];
        
        // Handle strings
        if ((c == '"' || c == '\'' || c == '`') && !in_regex) {
            if (!in_string) {
                in_string = true;
                string_delimiter = c;
                result += c;
            } else if (c == string_delimiter) {
                in_string = false;
                string_delimiter = 0;
                result += c;
            } else {
                result += c;
            }
            i++;
            continue;
        }
        
        if (in_string) {
            result += c;
            if (c == '\\' && i + 1 < jsCode.length()) {
                result += jsCode[++i]; // Skip escaped character
            }
            i++;
            continue;
        }
        
        // Handle comments
        if (c == '/' && i + 1 < jsCode.length()) {
            char next = jsCode[i + 1];
            if (next == '/') {
                // Single line comment
                while (i < jsCode.length() && jsCode[i] != '\n') i++;
                if (i < jsCode.length()) i++; // Skip newline
                continue;
            } else if (next == '*') {
                // Multi-line comment
                i += 2; // Skip /*
                while (i < jsCode.length()) {
                    if (jsCode[i] == '*' && i + 1 < jsCode.length() && jsCode[i + 1] == '/') {
                        i += 2;
                        break;
                    }
                    i++;
                }
                continue;
            }
        }
        
        // Handle regular expressions
        if (c == '/' && !in_regex && i > 0) {
            char prev = jsCode[i - 1];
            if (prev == '(' || prev == ',' || prev == '=' || prev == ':' || 
                prev == '[' || prev == '!' || prev == '&' || prev == '|' || 
                prev == '?' || prev == '+' || prev == '-' || prev == '~' || 
                prev == '*' || prev == '/' || prev == '{' || prev == '}' || prev == ';') {
                in_regex = true;
                result += c;
                i++;
                continue;
            }
        }
        
        if (in_regex) {
            result += c;
            if (c == '\\' && i + 1 < jsCode.length()) {
                result += jsCode[++i]; // Skip escaped character
            } else if (c == '/' && i > 0 && jsCode[i - 1] != '\\') {
                in_regex = false;
            }
            i++;
            continue;
        }
        
        // Handle whitespace
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            last_was_space = true;
            i++;
            continue;
        }
        
        // Tokenize for keywords and identifiers
        if (isalpha(c) || c == '_' || c == '$') {
            std::string token;
            size_t j = i;
            while (j < jsCode.length() && (isalnum(jsCode[j]) || jsCode[j] == '_' || jsCode[j] == '$')) {
                token += jsCode[j];
                j++;
            }
            // Insert space if previous token was a keyword or identifier
            if (!last_token.empty() && (is_keyword(last_token) || isalnum(last_token[0]) || last_token[0] == '_' || last_token[0] == '$')) {
                result += ' ';
            }
            result += token;
            last_token = token;
            i = j;
            last_was_space = false;
            continue;
        }
        // For all other characters
        result += c;
        last_token = std::string(1, c);
        last_was_space = false;
        i++;
    }
    return result;
} 