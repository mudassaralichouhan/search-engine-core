#!/bin/bash

# Setup script for JavaScript minification with Terser
# This script sets up both integrated and microservice approaches

set -e  # Exit on any error

echo "🗜️  Setting up JavaScript Minification with Terser"
echo "================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
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

# Check if running in the correct directory
if [ ! -f "package.json" ] || [ ! -f "docker/docker-compose.yml" ]; then
    print_error "Please run this script from the project root directory"
    exit 1
fi

print_status "Checking prerequisites..."

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    print_warning "Node.js not found locally, but it will be installed in Docker containers"
else
    NODE_VERSION=$(node --version)
    print_success "Node.js found: $NODE_VERSION"
fi

# Check if Docker is installed and running
if ! command -v docker &> /dev/null; then
    print_error "Docker is required but not installed. Please install Docker first."
    exit 1
fi

if ! docker info &> /dev/null; then
    print_error "Docker is not running. Please start Docker first."
    exit 1
fi

print_success "Docker is available and running"

# Install Node.js dependencies
print_status "Installing Node.js dependencies..."
if command -v npm &> /dev/null; then
    npm install
    print_success "Node.js dependencies installed locally"
else
    print_warning "npm not available locally, dependencies will be installed in containers"
fi

# Create necessary directories
print_status "Creating necessary directories..."
mkdir -p logs
mkdir -p temp
print_success "Directories created"

# Build Docker images
print_status "Building Docker images..."

# Build main application
print_status "Building search-engine container..."
docker-compose build search-engine

# Build JS minifier service
print_status "Building js-minifier service..."
docker-compose build js-minifier

print_success "All Docker images built successfully"

# Start services
print_status "Starting services..."
docker-compose up -d redis mongodb js-minifier

# Wait for services to be ready
print_status "Waiting for services to be ready..."

# Wait for Redis
until docker-compose exec redis redis-cli ping > /dev/null 2>&1; do
    print_status "Waiting for Redis..."
    sleep 2
done
print_success "Redis is ready"

# Wait for MongoDB
until docker-compose exec mongodb mongosh --eval "db.adminCommand('ping')" > /dev/null 2>&1; do
    print_status "Waiting for MongoDB..."
    sleep 2
done
print_success "MongoDB is ready"

# Wait for JS Minifier service
print_status "Waiting for JS Minifier service..."
until curl -s -f http://localhost:3002/health > /dev/null; do
    sleep 2
done
print_success "JS Minifier service is ready"

# Test the services
print_status "Testing JS minification..."

# Test the microservice
TEST_JS='function test() { console.log("Hello, World!"); var unused = 123; }'
RESPONSE=$(curl -s -X POST http://localhost:3002/minify \
    -H "Content-Type: application/json" \
    -d "{\"code\":\"$TEST_JS\",\"level\":\"advanced\"}")

if echo "$RESPONSE" | grep -q '"code"'; then
    print_success "JS Minifier service is working correctly"
    MINIFIED_CODE=$(echo "$RESPONSE" | grep -o '"code":"[^"]*"' | sed 's/"code":"//' | sed 's/"$//')
    print_status "Test minification result: $MINIFIED_CODE"
else
    print_warning "JS Minifier service test failed, but setup continues"
    print_status "Response: $RESPONSE"
fi

# Create example configuration files
print_status "Creating example configuration..."

cat > .env.example << 'EOF'
# JavaScript Minification Configuration
MINIFY_JS=true
MINIFY_JS_LEVEL=advanced

# Supported levels:
# - none: No minification
# - basic: Comment and whitespace removal only  
# - advanced: Full Terser optimization (recommended)

# Service URLs (for microservice approach)
JS_MINIFIER_SERVICE_URL=http://js-minifier:3002

# Other application settings
PORT=3000
MONGODB_URI=mongodb://admin:password123@mongodb:27017
SEARCH_REDIS_URI=tcp://redis:6379
EOF

print_success "Example configuration created (.env.example)"

# Create test script
print_status "Creating test script..."

cat > test_minification.sh << 'EOF'
#!/bin/bash

echo "Testing JavaScript Minification Setup"
echo "===================================="

# Test 1: Service Health
echo "1. Testing service health..."
curl -s http://localhost:3002/health | jq '.' || echo "Service not responding"

echo ""
echo "2. Testing basic minification..."
curl -s -X POST http://localhost:3002/minify \
    -H "Content-Type: application/json" \
    -d '{"code": "function hello(name) { console.log(\"Hello, \" + name + \"!\"); return name.toUpperCase(); }", "level": "basic"}' | jq '.stats'

echo ""
echo "3. Testing advanced minification..."
curl -s -X POST http://localhost:3002/minify \
    -H "Content-Type: application/json" \
    -d '{"code": "function hello(name) { console.log(\"Hello, \" + name + \"!\"); return name.toUpperCase(); }", "level": "advanced"}' | jq '.stats'

echo ""
echo "4. Testing batch processing..."
curl -s -X POST http://localhost:3002/minify/batch \
    -H "Content-Type: application/json" \
    -d '{"files": [{"name": "test1.js", "code": "function test1() { return 42; }"}, {"name": "test2.js", "code": "function test2() { return \"hello\"; }"}], "level": "advanced"}' | jq '.summary'

echo ""
echo "Test completed!"
EOF

chmod +x test_minification.sh
print_success "Test script created (test_minification.sh)"

# Show final status
print_status "Setup Summary:"
echo "=============="

docker-compose ps

echo ""
print_success "🎉 JavaScript Minification Setup Complete!"
echo ""
echo "Services available:"
echo "• Main application: http://localhost:3000"
echo "• JS Minifier API:  http://localhost:3002"
echo "• Redis:            localhost:6379"  
echo "• MongoDB:          localhost:27017"
echo ""
echo "Next steps:"
echo "1. Run './test_minification.sh' to test the setup"
echo "2. Check the documentation: README_JS_MINIFICATION.md"
echo "3. See examples in: examples/js_minification_example.cpp"
echo "4. Start the main application: docker-compose up search-engine"
echo ""
echo "Configuration options:"
echo "• Copy .env.example to .env and customize settings"
echo "• Modify docker/docker-compose.yml environment variables"
echo "• See README_JS_MINIFICATION.md for advanced options"
echo ""

print_status "View logs with: docker-compose logs -f [service-name]"
print_status "Stop services with: docker-compose down"

echo ""
print_success "Happy minifying! 🗜️"
