#!/bin/bash

# SPA Detection API Test Script
# Tests various websites to see if they are detected as SPAs

API_BASE="http://localhost:3000/api"

echo "=== SPA Detection API Test ==="
echo "Testing various websites for SPA detection..."
echo ""

# Test URLs - mix of SPAs and traditional sites
declare -a test_urls=(
    "https://digikala.com"
    "https://example.com"
    "https://reactjs.org"
    "https://vuejs.org"
    "https://angular.io"
    "https://httpbin.org/html"
    "https://github.com"
    "https://stackoverflow.com"
)

for url in "${test_urls[@]}"; do
    echo "Testing: $url"
    echo "----------------------------------------"
    
    # Make API call
    response=$(curl -s -X POST "$API_BASE/spa/detect" \
        -H "Content-Type: application/json" \
        -d "{
            \"url\": \"$url\",
            \"timeout\": 30000,
            \"userAgent\": \"Hatefbot/1.0\"
        }")
    
    # Extract key information
    is_spa=$(echo "$response" | jq -r '.spaDetection.isSpa // "unknown"')
    confidence=$(echo "$response" | jq -r '.spaDetection.confidence // 0')
    indicators=$(echo "$response" | jq -r '.spaDetection.indicators[]? // empty' | tr '\n' ', ' | sed 's/,$//')
    status_code=$(echo "$response" | jq -r '.httpStatusCode // "unknown"')
    content_size=$(echo "$response" | jq -r '.contentSize // 0')
    fetch_duration=$(echo "$response" | jq -r '.fetchDuration // 0')
    
    # Display results
    echo "SPA Detection: $is_spa"
    echo "Confidence: ${confidence}%"
    echo "HTTP Status: $status_code"
    echo "Content Size: ${content_size} bytes"
    echo "Fetch Duration: ${fetch_duration}ms"
    
    if [ ! -z "$indicators" ]; then
        echo "Indicators: $indicators"
    else
        echo "Indicators: None detected"
    fi
    
    # Check for errors
    if echo "$response" | jq -e '.error' > /dev/null; then
        error=$(echo "$response" | jq -r '.error')
        echo "ERROR: $error"
    fi
    
    echo ""
    sleep 2  # Rate limiting
done

echo "=== Test Complete ==="
echo ""
echo "API Endpoint: POST $API_BASE/spa/detect"
echo "Request Format:"
echo '{
    "url": "https://example.com",
    "timeout": 30000,
    "userAgent": "Custom Bot/1.0"
}'
echo ""
echo "Response includes:"
echo "- SPA detection result (true/false)"
echo "- Confidence score (0-100%)"
echo "- Detected frameworks/indicators"
echo "- HTTP status and content size"
echo "- Fetch duration and content preview" 