#!/bin/bash

# Test JavaScript Minification Caching
# This script tests the caching functionality for minified JS files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BASE_URL="http://localhost:3000"
CACHE_STATS_URL="$BASE_URL/api/cache/stats"
CACHE_INFO_URL="$BASE_URL/api/cache/info"
TEST_JS_FILE="public/test.js"

# Helper functions
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "\n${BLUE}================================${NC}"
    echo -e "${BLUE}  JS Minification Cache Test${NC}"
    echo -e "${BLUE}================================${NC}\n"
}

# Function to check if service is running
check_service() {
    print_info "Checking if search engine is running..."
    
    if ! curl -s "$BASE_URL" > /dev/null 2>&1; then
        print_error "Search engine is not running on $BASE_URL"
        print_info "Please start the service with: docker-compose up -d"
        exit 1
    fi
    
    print_success "Search engine is running"
}

# Function to create test JS file
create_test_file() {
    print_info "Creating test JavaScript file..."
    
    mkdir -p public
    
    cat > "$TEST_JS_FILE" << 'EOF'
// Test JavaScript file for caching
function calculateSum(a, b) {
    // This is a simple addition function
    var result = a + b;
    console.log("Calculating sum of " + a + " and " + b);
    return result;
}

function processArray(items) {
    // Process array items
    var processed = [];
    for (var i = 0; i < items.length; i++) {
        processed.push(items[i] * 2);
    }
    return processed;
}

// Export functions
window.calculateSum = calculateSum;
window.processArray = processArray;
EOF
    
    print_success "Test file created: $TEST_JS_FILE"
}

# Function to get cache stats
get_cache_stats() {
    local stats=$(curl -s "$CACHE_STATS_URL")
    echo "$stats"
}

# Function to parse JSON value
parse_json_value() {
    local json="$1"
    local key="$2"
    echo "$json" | grep -o "\"$key\":[^,}]*" | cut -d':' -f2 | tr -d '"' | tr -d ' '
}

# Function to test cache functionality
test_cache() {
    print_header
    
    check_service
    create_test_file
    
    print_info "Testing JavaScript minification caching..."
    
    # Get initial cache stats
    print_info "Getting initial cache stats..."
    initial_stats=$(get_cache_stats)
    initial_requests=$(parse_json_value "$initial_stats" "total_requests")
    initial_hits=$(parse_json_value "$initial_stats" "cache_hits")
    initial_misses=$(parse_json_value "$initial_stats" "cache_misses")
    
    print_info "Initial stats - Requests: $initial_requests, Hits: $initial_hits, Misses: $initial_misses"
    
    # First request (should be a cache miss)
    print_info "Making first request (should be cache miss)..."
    first_response=$(curl -s "$BASE_URL/test.js")
    
    if [ -z "$first_response" ]; then
        print_error "Failed to get response from test.js"
        exit 1
    fi
    
    print_success "First request completed"
    
    # Wait a moment for processing
    sleep 1
    
    # Get stats after first request
    stats_after_first=$(get_cache_stats)
    requests_after_first=$(parse_json_value "$stats_after_first" "total_requests")
    hits_after_first=$(parse_json_value "$stats_after_first" "cache_hits")
    misses_after_first=$(parse_json_value "$stats_after_first" "cache_misses")
    
    print_info "After first request - Requests: $requests_after_first, Hits: $hits_after_first, Misses: $misses_after_first"
    
    # Second request (should be a cache hit)
    print_info "Making second request (should be cache hit)..."
    second_response=$(curl -s "$BASE_URL/test.js")
    
    if [ -z "$second_response" ]; then
        print_error "Failed to get response from test.js on second request"
        exit 1
    fi
    
    print_success "Second request completed"
    
    # Wait a moment for processing
    sleep 1
    
    # Get final stats
    final_stats=$(get_cache_stats)
    final_requests=$(parse_json_value "$final_stats" "total_requests")
    final_hits=$(parse_json_value "$final_stats" "cache_hits")
    final_misses=$(parse_json_value "$final_stats" "cache_misses")
    hit_rate=$(parse_json_value "$final_stats" "cache_hit_rate")
    
    print_info "Final stats - Requests: $final_requests, Hits: $final_hits, Misses: $final_misses"
    print_info "Cache hit rate: $hit_rate"
    
    # Verify cache is working
    if [ "$final_misses" -gt "$initial_misses" ] && [ "$final_hits" -gt "$initial_hits" ]; then
        print_success "Cache is working! We have both misses and hits"
    else
        print_warning "Cache behavior unexpected. Check the implementation."
    fi
    
    # Test cache info endpoint
    print_info "Testing cache info endpoint..."
    cache_info=$(curl -s "$CACHE_INFO_URL")
    
    if echo "$cache_info" | grep -q "redis"; then
        print_success "Cache info endpoint working - Redis cache detected"
    else
        print_warning "Cache info endpoint response unexpected"
    fi
    
    # Performance comparison
    print_info "Performance comparison:"
    avg_min_time=$(parse_json_value "$final_stats" "avg_minification_time_ms")
    avg_cache_time=$(parse_json_value "$final_stats" "avg_cache_time_ms")
    
    echo "  - Average minification time: ${avg_min_time}ms"
    echo "  - Average cache access time: ${avg_cache_time}ms"
    
    if [ -n "$avg_min_time" ] && [ -n "$avg_cache_time" ]; then
        if (( $(echo "$avg_cache_time < $avg_min_time" | bc -l) )); then
            print_success "Cache is faster than minification! (${avg_cache_time}ms vs ${avg_min_time}ms)"
        else
            print_warning "Cache performance needs investigation"
        fi
    fi
    
    print_success "Cache test completed successfully!"
}

# Function to clean up
cleanup() {
    print_info "Cleaning up test files..."
    if [ -f "$TEST_JS_FILE" ]; then
        rm -f "$TEST_JS_FILE"
        print_success "Test file removed"
    fi
}

# Main execution
main() {
    case "${1:-test}" in
        "test")
            test_cache
            ;;
        "stats")
            print_info "Getting current cache stats..."
            stats=$(get_cache_stats)
            echo "$stats" | python3 -m json.tool 2>/dev/null || echo "$stats"
            ;;
        "clear")
            print_info "Clearing cache stats..."
            curl -s -X POST "$BASE_URL/api/cache/clear"
            print_success "Cache stats cleared"
            ;;
        "info")
            print_info "Getting cache info..."
            info=$(curl -s "$CACHE_INFO_URL")
            echo "$info" | python3 -m json.tool 2>/dev/null || echo "$info"
            ;;
        *)
            echo "Usage: $0 [test|stats|clear|info]"
            echo "  test  - Run full cache test (default)"
            echo "  stats - Show current cache statistics"
            echo "  clear - Clear cache statistics"
            echo "  info  - Show cache configuration info"
            exit 1
            ;;
    esac
}

# Set up cleanup on exit
trap cleanup EXIT

# Run main function
main "$@"
