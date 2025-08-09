#pragma once

// Failure classification types used in public crawl results
enum class FailureType {
    TEMPORARY,    // Retry with exponential backoff
    RATE_LIMITED, // Retry with longer delay (respect rate limits)
    PERMANENT,    // Don't retry (404, 403, DNS doesn't exist, etc.)
    UNKNOWN       // Retry with caution (limited attempts)
};


