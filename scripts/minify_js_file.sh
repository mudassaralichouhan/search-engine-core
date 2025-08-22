#!/bin/bash

# JS Minifier File Processing Script
# Usage: ./minify_js_file.sh <input_file.js> [output_file.js] [level]

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_header() {
    echo -e "${PURPLE}$1${NC}"
}

print_stats() {
    echo -e "${CYAN}$1${NC}"
}

# Function to show usage
show_usage() {
    echo "🗜️  JS Minifier File Processor"
    echo "=============================="
    echo ""
    echo "Usage: $0 <input_file.js> [output_file.js] [level]"
    echo ""
    echo "Parameters:"
    echo "  input_file.js    Path to the JavaScript file to minify (required)"
    echo "  output_file.js   Path for the minified output (optional)"
    echo "  level           Minification level: basic, advanced, aggressive (default: advanced)"
    echo ""
    echo "Examples:"
    echo "  $0 script.js"
    echo "  $0 script.js script.min.js"
    echo "  $0 script.js script.min.js aggressive"
    echo "  $0 script.js script.min.js basic"
    echo ""
    echo "Minification Levels:"
    echo "  basic      - Remove comments and whitespace"
    echo "  advanced   - Variable renaming and dead code elimination"
    echo "  aggressive - Maximum compression with unsafe optimizations"
    echo ""
}

# Function to check if JS minifier service is available
check_service() {
    print_info "Checking JS Minifier service..."
    
    if ! curl -s http://localhost:3002/health > /dev/null 2>&1; then
        print_error "JS Minifier service is not running!"
        print_info "Please start the service with: docker-compose up js-minifier -d"
        exit 1
    fi
    
    print_success "JS Minifier service is available"
}

# Function to get file size in human readable format
get_file_size() {
    local file="$1"
    if [ -f "$file" ]; then
        local size=$(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null)
        if [ "$size" -gt 1048576 ]; then
            echo "$(echo "scale=2; $size/1048576" | bc)MB"
        elif [ "$size" -gt 1024 ]; then
            echo "$(echo "scale=2; $size/1024" | bc)KB"
        else
            echo "${size}B"
        fi
    else
        echo "0B"
    fi
}

# Function to minify JavaScript file
minify_file() {
    local input_file="$1"
    local output_file="$2"
    local level="$3"
    
    print_info "Processing: $input_file"
    print_info "Level: $level"
    
    # Read the input file
    if [ ! -f "$input_file" ]; then
        print_error "Input file not found: $input_file"
        exit 1
    fi
    
    local original_size=$(stat -c%s "$input_file" 2>/dev/null || stat -f%z "$input_file" 2>/dev/null)
    print_stats "Original size: ${original_size}B ($(get_file_size "$input_file"))"
    
    # Choose method based on file size (use file upload for large files)
    local file_size_kb=$(echo "scale=2; $original_size / 1024" | bc)
    
    if (( $(echo "$file_size_kb > 100" | bc -l) )); then
        print_info "Large file detected (${file_size_kb}KB), using file upload method..."
        minify_file_upload "$input_file" "$output_file" "$level"
    else
        print_info "Small file detected (${file_size_kb}KB), using JSON payload method..."
        minify_file_json "$input_file" "$output_file" "$level"
    fi
}

# Function to minify using JSON payload (for small files)
minify_file_json() {
    local input_file="$1"
    local output_file="$2"
    local level="$3"
    
    # Create JSON payload
    local js_content=$(cat "$input_file")
    local json_payload=$(cat <<EOF
{
  "code": $(echo "$js_content" | jq -Rs .),
  "level": "$level"
}
EOF
)
    
    # Send request to minifier service
    print_info "Sending JSON payload to minifier service..."
    
    local start_time=$(date +%s%N)
    
    local response=$(curl -s -X POST http://localhost:3002/minify/json \
        -H "Content-Type: application/json" \
        -d "$json_payload")
    
    local end_time=$(date +%s%N)
    local processing_time=$(( (end_time - start_time) / 1000000 ))  # Convert to milliseconds
    
    # Check if request was successful
    if echo "$response" | jq -e '.error' > /dev/null 2>&1; then
        local error_msg=$(echo "$response" | jq -r '.error // .message // "Unknown error"')
        print_error "Minification failed: $error_msg"
        exit 1
    fi
    
    # Extract minified code and stats
    local minified_code=$(echo "$response" | jq -r '.code')
    local minified_size=$(echo "$response" | jq -r '.stats.minified_size')
    local compression_ratio=$(echo "$response" | jq -r '.stats.compression_ratio')
    
    # Write minified code to output file
    echo "$minified_code" > "$output_file"
    
    print_success "Minification completed successfully!"
    print_stats "Minified size: ${minified_size}B ($(get_file_size "$output_file"))"
    print_stats "Compression ratio: $compression_ratio"
    print_stats "Processing time: ${processing_time}ms"
    print_success "Output saved to: $output_file"
}

# Function to minify using file upload (for large files)
minify_file_upload() {
    local input_file="$1"
    local output_file="$2"
    local level="$3"
    
    # Send file upload request to minifier service
    print_info "Sending file upload to minifier service..."
    
    local start_time=$(date +%s%N)
    
    local response=$(curl -s -X POST http://localhost:3002/minify/upload \
        -F "jsfile=@$input_file" \
        -F "level=$level")
    
    local end_time=$(date +%s%N)
    local processing_time=$(( (end_time - start_time) / 1000000 ))  # Convert to milliseconds
    
    # Check if request was successful
    if echo "$response" | jq -e '.error' > /dev/null 2>&1; then
        local error_msg=$(echo "$response" | jq -r '.error // .message // "Unknown error"')
        print_error "Minification failed: $error_msg"
        exit 1
    fi
    
    # Extract minified code and stats
    local minified_code=$(echo "$response" | jq -r '.code')
    local minified_size=$(echo "$response" | jq -r '.stats.minified_size')
    local compression_ratio=$(echo "$response" | jq -r '.stats.compression_ratio')
    
    # Write minified code to output file
    echo "$minified_code" > "$output_file"
    
    print_success "Minification completed successfully!"
    print_stats "Minified size: ${minified_size}B ($(get_file_size "$output_file"))"
    print_stats "Compression ratio: $compression_ratio"
    print_stats "Processing time: ${processing_time}ms"
    print_success "Output saved to: $output_file"
}

# Function to show file comparison
show_comparison() {
    local input_file="$1"
    local output_file="$2"
    
    echo ""
    print_header "📊 File Comparison"
    echo "======================"
    
    local input_size=$(stat -c%s "$input_file" 2>/dev/null || stat -f%z "$input_file" 2>/dev/null)
    local output_size=$(stat -c%s "$output_file" 2>/dev/null || stat -f%z "$output_file" 2>/dev/null)
    local savings=$((input_size - output_size))
    local savings_percent=$(echo "scale=2; $savings * 100 / $input_size" | bc)
    
    echo "Original:  $(get_file_size "$input_file") ($input_size bytes)"
    echo "Minified:  $(get_file_size "$output_file") ($output_size bytes)"
    echo "Savings:   $(get_file_size <(echo $savings)) ($savings bytes, ${savings_percent}%)"
    
    # Show first few lines of both files
    echo ""
    print_header "📄 Original file (first 3 lines):"
    echo "----------------------------------------"
    head -3 "$input_file" | sed 's/^/  /'
    
    echo ""
    print_header "📄 Minified file (first 3 lines):"
    echo "----------------------------------------"
    head -3 "$output_file" | sed 's/^/  /'
}

# Main script
main() {
    # Check if jq is available
    if ! command -v jq &> /dev/null; then
        print_error "jq is required but not installed. Please install jq first."
        print_info "Ubuntu/Debian: sudo apt-get install jq"
        print_info "macOS: brew install jq"
        exit 1
    fi
    
    # Check if bc is available
    if ! command -v bc &> /dev/null; then
        print_error "bc is required but not installed. Please install bc first."
        print_info "Ubuntu/Debian: sudo apt-get install bc"
        print_info "macOS: brew install bc"
        exit 1
    fi
    
    # Parse command line arguments
    if [ $# -eq 0 ]; then
        show_usage
        exit 1
    fi
    
    local input_file="$1"
    local output_file="$2"
    local level="${3:-advanced}"
    
    # Validate input file
    if [ ! -f "$input_file" ]; then
        print_error "Input file not found: $input_file"
        exit 1
    fi
    
    # Check file extension
    if [[ ! "$input_file" =~ \.js$ ]]; then
        print_warning "File doesn't have .js extension: $input_file"
    fi
    
    # Generate output filename if not provided
    if [ -z "$output_file" ]; then
        local base_name=$(basename "$input_file" .js)
        local dir_name=$(dirname "$input_file")
        output_file="${dir_name}/${base_name}.min.js"
        print_info "Output file not specified, using: $output_file"
    fi
    
    # Validate minification level
    case "$level" in
        basic|advanced|aggressive)
            ;;
        *)
            print_error "Invalid minification level: $level"
            print_info "Valid levels: basic, advanced, aggressive"
            exit 1
            ;;
    esac
    
    # Check service availability
    check_service
    
    # Show processing info
    print_header "🗜️  Starting JavaScript Minification"
    echo "=========================================="
    
    # Minify the file
    minify_file "$input_file" "$output_file" "$level"
    
    # Show comparison
    show_comparison "$input_file" "$output_file"
    
    echo ""
    print_success "🎉 Minification completed successfully!"
}

# Run main function with all arguments
main "$@"
