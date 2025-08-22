#!/bin/bash

# Clean Microservice Setup Script
# This script sets up ONLY the microservice architecture (recommended)

set -e  # Exit on any error

echo "🗜️  Setting up Clean Microservice Architecture for JS Minification"
echo "================================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check if running in the correct directory
if [ ! -f "docker/docker-compose.yml" ]; then
    print_error "Please run this script from the project root directory"
    exit 1
fi

print_status "Checking prerequisites..."

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

# Clean up any existing containers
print_status "Cleaning up existing containers..."
docker-compose down --remove-orphans 2>/dev/null || true

# Build images
print_status "Building Docker images for microservice architecture..."

# Build JS minifier service (separate container)
print_status "Building dedicated JS minifier service..."
docker-compose build js-minifier

# Build main application (your existing Dockerfile with Node.js for integrated fallback)
print_status "Building main search engine..."
docker-compose build search-engine

print_success "All Docker images built successfully"

# Start services in the correct order
print_status "Starting services..."

# Start infrastructure first
print_status "Starting Redis and MongoDB..."
docker-compose up -d redis mongodb

# Start JS minifier service
print_status "Starting JS minifier service..."
docker-compose up -d js-minifier

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
MAX_ATTEMPTS=30
ATTEMPT=0
until curl -s -f http://localhost:3002/health > /dev/null; do
    ATTEMPT=$((ATTEMPT + 1))
    if [ $ATTEMPT -ge $MAX_ATTEMPTS ]; then
        print_error "JS Minifier service failed to start after $MAX_ATTEMPTS attempts"
        docker-compose logs js-minifier
        exit 1
    fi
    print_status "Attempt $ATTEMPT/$MAX_ATTEMPTS - waiting for JS Minifier service..."
    sleep 2
done
print_success "JS Minifier service is ready"

# Test the microservice
print_status "Testing microservice functionality..."

# Test service health
HEALTH_RESPONSE=$(curl -s http://localhost:3002/health)
if echo "$HEALTH_RESPONSE" | grep -q '"status":"healthy"'; then
    print_success "JS Minifier service health check passed"
else
    print_warning "Health check returned unexpected response: $HEALTH_RESPONSE"
fi

# Test minification
TEST_JS='function test() { console.log("Hello, World!"); var unused = 123; /* comment */ }'
RESPONSE=$(curl -s -X POST http://localhost:3002/minify \
    -H "Content-Type: application/json" \
    -d "{\"code\":\"$TEST_JS\",\"level\":\"advanced\"}")

if echo "$RESPONSE" | grep -q '"code"'; then
    MINIFIED_CODE=$(echo "$RESPONSE" | grep -o '"code":"[^"]*"' | sed 's/"code":"//' | sed 's/"$//')
    ORIGINAL_SIZE=$(echo -n "$TEST_JS" | wc -c)
    MINIFIED_SIZE=$(echo -n "$MINIFIED_CODE" | wc -c)
    SAVINGS=$((ORIGINAL_SIZE - MINIFIED_SIZE))
    
    print_success "Minification test passed!"
    print_status "Original: $ORIGINAL_SIZE bytes → Minified: $MINIFIED_SIZE bytes (saved $SAVINGS bytes)"
    print_status "Result: $MINIFIED_CODE"
else
    print_warning "Minification test failed: $RESPONSE"
fi

# Show final status
echo ""
print_success "🎉 Clean Microservice Architecture Setup Complete!"
echo ""
echo "🏗️  MICROSERVICE ARCHITECTURE (Recommended)"
echo "==========================================="
docker-compose ps

echo ""
echo "📊 Service Endpoints:"
echo "• JS Minifier API:  http://localhost:3002"
echo "  - Health:         http://localhost:3002/health"
echo "  - Config:         http://localhost:3002/config"
echo "  - Minify:         POST http://localhost:3002/minify"
echo "  - Batch:          POST http://localhost:3002/minify/batch"
echo "• Redis:            localhost:6379"  
echo "• MongoDB:          localhost:27017"
echo ""

# Create test script
cat > test_microservice.sh << 'EOF'
#!/bin/bash
echo "🧪 Testing Microservice Architecture"
echo "===================================="

echo "1. Service Health:"
curl -s http://localhost:3002/health | jq '.'

echo -e "\n2. Service Config:"
curl -s http://localhost:3002/config | jq '.available_levels'

echo -e "\n3. Basic Minification:"
curl -s -X POST http://localhost:3002/minify \
    -H "Content-Type: application/json" \
    -d '{"code": "function hello(name) { console.log(\"Hello, \" + name + \"!\"); return name; }", "level": "basic"}' \
    | jq '.stats'

echo -e "\n4. Advanced Minification:"
curl -s -X POST http://localhost:3002/minify \
    -H "Content-Type: application/json" \
    -d '{"code": "function hello(name) { console.log(\"Hello, \" + name + \"!\"); return name; }", "level": "advanced"}' \
    | jq '.stats'

echo -e "\n✅ Microservice tests completed!"
EOF

chmod +x test_microservice.sh
print_success "Test script created: test_microservice.sh"

echo ""
echo "🚀 Next Steps:"
echo "1. Run './test_microservice.sh' to test the microservice"
echo "2. Start your main application: docker-compose up search-engine"
echo "3. Use JsMinifierClient in your C++ code to call the service"
echo ""

echo "📖 Code Usage:"
echo "C++ Integration:"
echo '  JsMinifierClient client("http://js-minifier:3002");'
echo '  std::string minified = client.minify(jsCode, "advanced");'
echo ""

echo "🔧 Management Commands:"
echo "• View logs:        docker-compose logs -f [service]"
echo "• Stop services:    docker-compose down"
echo "• Restart service:  docker-compose restart js-minifier"
echo "• Scale service:    docker-compose up --scale js-minifier=3"
echo ""

print_success "🎯 Clean microservice architecture ready! No Node.js bloat in your main container!"
