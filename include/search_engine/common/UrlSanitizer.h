#pragma once

#include <string>

namespace search_engine::common {

// Remove invisible/formatting Unicode codepoints commonly found in copy/pasted URLs
// and strip ASCII control characters and surrounding ASCII whitespace.
// Specifically removes: U+200B, U+200C, U+200D, U+2060, U+FEFF and bytes < 0x20 or 0x7F.
// Also trims leading/trailing spaces, tabs, CR, LF.
std::string sanitizeUrl(const std::string& input);

// Produce a compact hex dump of the given string for logging/debugging.
// Example: "68 74 74 70 73 3a 2f ..."
std::string hexDump(const std::string& input);

} // namespace search_engine::common


