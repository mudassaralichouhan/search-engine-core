#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct EmailTemplate {
    std::string subject;
    std::string htmlBody;
    std::string textBody;
    std::vector<std::string> requiredVariables;
    
    std::string render(const json& variables) const;
};

struct EmailConfig {
    std::string smtpHost = "smtp.gmail.com";
    int smtpPort = 587;
    std::string username;
    std::string password;
    std::string fromEmail;
    std::string fromName = "Search Engine";
    bool useTLS = true;
    bool debug = false;
};

struct EmailResult {
    bool success = false;
    std::string message;
    std::string messageId;
    std::chrono::system_clock::time_point sentAt;
};

class EmailService {
public:
    EmailService(const EmailConfig& config = EmailConfig{});
    ~EmailService() = default;
    
    // Template management
    void addTemplate(const std::string& name, const EmailTemplate& emailTemplate);
    bool hasTemplate(const std::string& name) const;
    
    // Email sending
    EmailResult sendEmail(const std::string& to, const std::string& subject, 
                         const std::string& htmlBody, const std::string& textBody = "");
    
    EmailResult sendTemplateEmail(const std::string& to, const std::string& templateName, 
                                 const json& variables);
    
    // Configuration
    void updateConfig(const EmailConfig& config);
    EmailConfig getConfig() const { return config_; }
    
    // Test connectivity
    bool testConnection();
    
private:
    EmailConfig config_;
    std::unordered_map<std::string, EmailTemplate> templates_;
    
    void initializeDefaultTemplates();
    std::string generateMessageId();
};
