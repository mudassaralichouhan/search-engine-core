#!/bin/bash

# SPA URL Testing Script
# Usage: ./test_spa_url.sh digikala.com
# Usage: ./test_spa_url.sh digikala.com reactjs.org istgah.com
# Usage: ./test_spa_url.sh "[PageFetcher][SPA]" digikala.com  # run specific test pattern

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default test pattern
TEST_PATTERN="[PageFetcher][SPA][Parameterized]"

# Parse arguments
ARGS=()
for arg in "$@"; do
    if [[ "$arg" == "["*"]" ]]; then
        # This is a test pattern
        TEST_PATTERN="$arg"
    else
        # This is a URL
        ARGS+=("$arg")
    fi
done

# Check if we have URLs to test
if [ ${#ARGS[@]} -eq 0 ]; then
    echo -e "${RED}❌ Error: No URLs provided${NC}"
    echo ""
    echo -e "${YELLOW}Usage examples:${NC}"
    echo "  ./test_spa_url.sh digikala.com"
    echo "  ./test_spa_url.sh digikala.com reactjs.org"
    echo "  ./test_spa_url.sh \"[PageFetcher][SPA]\" digikala.com"
    echo ""
    echo -e "${YELLOW}Show full HTML content:${NC}"
    echo "  showhtml=true ./test_spa_url.sh istgah.com"
    echo "  showhtml=true ./test_spa_url.sh digikala.com reactjs.org"
    echo ""
    echo -e "${BLUE}Available test patterns:${NC}"
    echo "  [PageFetcher][SPA]                    - All SPA tests"
    echo "  [PageFetcher][SPA][Parameterized]     - Parameterized URL tests (default)"
    echo "  [PageFetcher][SPA][Integration]       - Integration tests with real URLs"
    echo ""
    echo -e "${BLUE}Environment variables:${NC}"
    echo "  showhtml=true                         - Show full HTML content (default: false)"
    exit 1
fi

# Join URLs with comma
URL_LIST=$(IFS=,; echo "${ARGS[*]}")

echo -e "${BLUE}🧪 Testing SPA Detection${NC}"
echo -e "${YELLOW}Test Pattern:${NC} $TEST_PATTERN"
echo -e "${YELLOW}URLs:${NC} $URL_LIST"

# Show HTML display status
if [ "$showhtml" = "true" ]; then
    echo -e "${YELLOW}HTML Display:${NC} ${GREEN}✅ ENABLED${NC} (showing full HTML content)"
else
    echo -e "${YELLOW}HTML Display:${NC} ${RED}❌ DISABLED${NC} (use showhtml=true to enable)"
fi
echo ""

# Build the project first
echo -e "${BLUE}🔨 Building tests...${NC}"
if ! cmake --build build --parallel 4 > /dev/null 2>&1; then
    echo -e "${RED}❌ Build failed${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Build successful${NC}"
echo ""

# Run the tests with URLs
echo -e "${BLUE}🚀 Running SPA detection tests...${NC}"
echo ""

export CMD_LINE_TEST_URL="$URL_LIST"
./build/tests/crawler/crawler_tests "$TEST_PATTERN"

echo ""
echo -e "${GREEN}✅ Test completed!${NC}" 