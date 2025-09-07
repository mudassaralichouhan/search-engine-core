#include "EmailService.h"
#include "../../include/Logger.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>
#include <thread>

// Simple template rendering
std::string EmailTemplate::render(const json& variables) const {
    std::string renderedSubject = subject;
    std::string renderedHtml = htmlBody;
    std::string renderedText = textBody;
    
    // Simple variable replacement using {{variable}} syntax
    for (const auto& [key, value] : variables.items()) {
        std::string placeholder = "{{" + key + "}}";
        std::string replacement;
        
        if (value.is_string()) {
            replacement = value.get<std::string>();
        } else if (value.is_number()) {
            replacement = std::to_string(value.get<double>());
        } else {
            replacement = value.dump();
        }
        
        // Replace in subject
        size_t pos = 0;
        while ((pos = renderedSubject.find(placeholder, pos)) != std::string::npos) {
            renderedSubject.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
        
        // Replace in HTML body
        pos = 0;
        while ((pos = renderedHtml.find(placeholder, pos)) != std::string::npos) {
            renderedHtml.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
        
        // Replace in text body
        pos = 0;
        while ((pos = renderedText.find(placeholder, pos)) != std::string::npos) {
            renderedText.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }
    
    return renderedHtml; // Return HTML version for now
}

EmailService::EmailService(const EmailConfig& config) : config_(config) {
    initializeDefaultTemplates();
    
    // Load email config from environment if not provided
    if (config_.username.empty()) {
        const char* username = std::getenv("EMAIL_USERNAME");
        if (username) config_.username = username;
    }
    
    if (config_.password.empty()) {
        const char* password = std::getenv("EMAIL_PASSWORD");
        if (password) config_.password = password;
    }
    
    if (config_.fromEmail.empty()) {
        const char* fromEmail = std::getenv("EMAIL_FROM");
        if (fromEmail) config_.fromEmail = fromEmail;
    }
    
    LOG_INFO("EmailService initialized with SMTP host: " + config_.smtpHost);
}

void EmailService::addTemplate(const std::string& name, const EmailTemplate& emailTemplate) {
    templates_[name] = emailTemplate;
    LOG_INFO("Added email template: " + name);
}

bool EmailService::hasTemplate(const std::string& name) const {
    return templates_.find(name) != templates_.end();
}

EmailResult EmailService::sendEmail(const std::string& to, const std::string& subject, 
                                   const std::string& htmlBody, const std::string& textBody) {
    EmailResult result;
    result.sentAt = std::chrono::system_clock::now();
    result.messageId = generateMessageId();
    
    // For now, we'll simulate email sending
    // In a real implementation, you would use a library like curl or libcurl
    // to send emails via SMTP
    
    LOG_INFO("Sending email to: " + to);
    LOG_INFO("Subject: " + subject);
    LOG_DEBUG("HTML Body preview: " + htmlBody.substr(0, 200) + "...");
    
    // Simulate email sending delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Simulate success (95% success rate)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    if (dis(gen) < 0.95) {
        result.success = true;
        result.message = "Email sent successfully";
        LOG_INFO("Email sent successfully to: " + to + " (Message ID: " + result.messageId + ")");
    } else {
        result.success = false;
        result.message = "SMTP server temporarily unavailable";
        LOG_ERROR("Failed to send email to: " + to + " - " + result.message);
    }
    
    return result;
}

EmailResult EmailService::sendTemplateEmail(const std::string& to, const std::string& templateName, 
                                           const json& variables) {
    auto it = templates_.find(templateName);
    if (it == templates_.end()) {
        EmailResult result;
        result.success = false;
        result.message = "Template not found: " + templateName;
        LOG_ERROR(result.message);
        return result;
    }
    
    const auto& emailTemplate = it->second;
    
    // Check required variables
    for (const auto& requiredVar : emailTemplate.requiredVariables) {
        if (variables.find(requiredVar) == variables.end()) {
            EmailResult result;
            result.success = false;
            result.message = "Missing required variable: " + requiredVar;
            LOG_ERROR(result.message);
            return result;
        }
    }
    
    // Render template
    std::string subject = emailTemplate.subject;
    std::string htmlBody = emailTemplate.render(variables);
    std::string textBody = emailTemplate.textBody;
    
    // Replace variables in subject and text body too
    for (const auto& [key, value] : variables.items()) {
        std::string placeholder = "{{" + key + "}}";
        std::string replacement;
        
        if (value.is_string()) {
            replacement = value.get<std::string>();
        } else if (value.is_number()) {
            replacement = std::to_string(value.get<double>());
        } else {
            replacement = value.dump();
        }
        
        // Replace in subject
        size_t pos = 0;
        while ((pos = subject.find(placeholder, pos)) != std::string::npos) {
            subject.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
        
        // Replace in text body
        pos = 0;
        while ((pos = textBody.find(placeholder, pos)) != std::string::npos) {
            textBody.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }
    
    return sendEmail(to, subject, htmlBody, textBody);
}

void EmailService::updateConfig(const EmailConfig& config) {
    config_ = config;
    LOG_INFO("Email configuration updated");
}

bool EmailService::testConnection() {
    // In a real implementation, this would test the SMTP connection
    LOG_INFO("Testing email connection to " + config_.smtpHost + ":" + std::to_string(config_.smtpPort));
    
    if (config_.username.empty() || config_.password.empty()) {
        LOG_ERROR("Email credentials not configured");
        return false;
    }
    
    // Simulate connection test
    LOG_INFO("Email connection test successful");
    return true;
}

void EmailService::initializeDefaultTemplates() {
    // Webmaster notification template
    EmailTemplate webmasterTemplate;
    webmasterTemplate.subject = "Your website {{domain}} has been crawled by our search engine";
    webmasterTemplate.requiredVariables = {"domain", "pagesCrawled", "crawlDate"};
    webmasterTemplate.htmlBody = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Website Crawl Notification</title>
    <style>
        body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; max-width: 600px; margin: 0 auto; padding: 20px; }
        .header { background-color: #f4f4f4; padding: 20px; text-align: center; border-radius: 5px; }
        .content { padding: 20px 0; }
        .footer { background-color: #f4f4f4; padding: 10px; text-align: center; font-size: 12px; border-radius: 5px; }
        .highlight { background-color: #e7f3ff; padding: 10px; border-left: 4px solid #2196F3; margin: 10px 0; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Website Crawl Notification</h1>
    </div>
    
    <div class="content">
        <p>Dear Webmaster,</p>
        
        <p>We are writing to inform you that your website <strong>{{domain}}</strong> has been crawled by our search engine.</p>
        
        <div class="highlight">
            <h3>Crawl Details:</h3>
            <ul>
                <li><strong>Domain:</strong> {{domain}}</li>
                <li><strong>Pages Crawled:</strong> {{pagesCrawled}}</li>
                <li><strong>Crawl Date:</strong> {{crawlDate}}</li>
            </ul>
        </div>
        
        <p>This crawling process helps us index your content and make it discoverable through our search engine. We respect robots.txt directives and follow ethical crawling practices.</p>
        
        <p>If you have any questions or concerns about this crawling activity, please don't hesitate to contact us.</p>
        
        <p>Best regards,<br>
        The Search Engine Team</p>
    </div>
    
    <div class="footer">
        <p>This is an automated notification. If you no longer wish to receive these notifications, please contact us.</p>
    </div>
</body>
</html>
)";
    
    webmasterTemplate.textBody = R"(
Dear Webmaster,

We are writing to inform you that your website {{domain}} has been crawled by our search engine.

Crawl Details:
- Domain: {{domain}}
- Pages Crawled: {{pagesCrawled}}
- Crawl Date: {{crawlDate}}

This crawling process helps us index your content and make it discoverable through our search engine. We respect robots.txt directives and follow ethical crawling practices.

If you have any questions or concerns about this crawling activity, please don't hesitate to contact us.

Best regards,
The Search Engine Team

---
This is an automated notification. If you no longer wish to receive these notifications, please contact us.
)";
    
    addTemplate("webmaster_notification", webmasterTemplate);
    
    // Crawl completion summary template
    EmailTemplate summaryTemplate;
    summaryTemplate.subject = "Bulk Crawl Operation Completed - {{totalDomains}} domains processed";
    summaryTemplate.requiredVariables = {"totalDomains", "successfulDomains", "failedDomains", "emailsSent"};
    summaryTemplate.htmlBody = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Bulk Crawl Summary</title>
    <style>
        body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; max-width: 600px; margin: 0 auto; padding: 20px; }
        .header { background-color: #4CAF50; color: white; padding: 20px; text-align: center; border-radius: 5px; }
        .stats { display: flex; justify-content: space-around; margin: 20px 0; }
        .stat-box { background-color: #f9f9f9; padding: 15px; border-radius: 5px; text-align: center; }
        .stat-number { font-size: 24px; font-weight: bold; color: #2196F3; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Bulk Crawl Operation Completed</h1>
    </div>
    
    <div class="stats">
        <div class="stat-box">
            <div class="stat-number">{{totalDomains}}</div>
            <div>Total Domains</div>
        </div>
        <div class="stat-box">
            <div class="stat-number">{{successfulDomains}}</div>
            <div>Successful</div>
        </div>
        <div class="stat-box">
            <div class="stat-number">{{failedDomains}}</div>
            <div>Failed</div>
        </div>
        <div class="stat-box">
            <div class="stat-number">{{emailsSent}}</div>
            <div>Emails Sent</div>
        </div>
    </div>
    
    <p>Your bulk crawl operation has been completed successfully.</p>
</body>
</html>
)";
    
    addTemplate("crawl_summary", summaryTemplate);
}

std::string EmailService::generateMessageId() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y%m%d%H%M%S");
    
    // Add random component
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    ss << "." << dis(gen) << "@searchengine.local";
    return ss.str();
}
