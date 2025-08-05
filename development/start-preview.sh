#!/bin/bash

# Start the preview server for template development
echo "🚀 Starting Template Preview Server..."
echo ""
echo "This server allows you to preview templates with test data"
echo "without affecting the production server."
echo ""

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo "❌ Node.js is not installed. Please install Node.js first."
    exit 1
fi

# Navigate to development directory
cd "$(dirname "$0")"

# Run the preview server
node preview-server.js