#pragma once

#include <string>
#include <chrono>
#include <optional>

namespace search_engine {
namespace storage {

enum class SponsorStatus {
    PENDING,        // Form submitted, waiting for payment
    VERIFIED,       // Payment verified and confirmed
    REJECTED,       // Application rejected
    CANCELLED       // Sponsor cancelled
};

struct SponsorProfile {
    // Unique identifier (MongoDB ObjectId will be auto-generated)
    std::optional<std::string> id;
    
    // Form data - required fields
    std::string fullName;
    std::string email;
    std::string mobile;
    std::string plan;           // tier/plan selected
    double amount;              // amount in USD or IRR
    
    // Form data - optional fields  
    std::optional<std::string> company;
    
    // Backend tracking data
    std::string ipAddress;
    std::string userAgent;
    std::chrono::system_clock::time_point submissionTime;
    std::chrono::system_clock::time_point lastModified;
    
    // Status and processing
    SponsorStatus status;
    std::optional<std::string> notes;           // Admin notes
    std::optional<std::string> paymentReference;  // Bank reference number
    std::optional<std::chrono::system_clock::time_point> paymentDate;
    
    // Financial tracking
    std::string currency;       // "USD", "IRR", "BTC"
    std::optional<std::string> bankAccountInfo;  // Which bank account was provided to user
    std::optional<std::string> transactionId;    // Payment gateway transaction ID
};

// Bank information for sponsor payments
struct BankInfo {
    std::string bankName;
    std::string accountNumber;
    std::string iban;
    std::string accountHolder;
    std::string swift;          // For international transfers
    std::string currency;
    std::optional<std::string> notes;
};

} // namespace storage
} // namespace search_engine
