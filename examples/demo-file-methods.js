const http = require('http');
const fs = require('fs');

console.log('🧪 Demonstrating Different File Receiving Methods');
console.log('=' .repeat(55));

// Sample JavaScript code
const sampleCode = `
// E-commerce product calculator
class ProductCalculator {
    constructor(taxRate = 0.08) {
        this.taxRate = taxRate;
        this.items = [];
    }
    
    addItem(product, quantity = 1) {
        const existingItem = this.items.find(item => item.product.id === product.id);
        if (existingItem) {
            existingItem.quantity += quantity;
            console.log('Updated quantity for:', product.name);
        } else {
            this.items.push({ product, quantity });
            console.log('Added new item:', product.name);
        }
        
        // This will be removed in advanced minification
        const debugInfo = 'Debug information';
        debugger;
    }
    
    calculateTotal() {
        let subtotal = 0;
        for (const item of this.items) {
            subtotal += item.product.price * item.quantity;
        }
        
        const tax = subtotal * this.taxRate;
        return {
            subtotal: subtotal,
            tax: tax,
            total: subtotal + tax
        };
    }
}

// Usage example
const calculator = new ProductCalculator();
calculator.addItem({ id: 1, name: 'Widget', price: 19.99 }, 2);
const result = calculator.calculateTotal();
console.log('Total:', result.total);
`;

// Test Methods
async function testMethod1_JSON() {
    console.log('\\n📝 Method 1: JSON Payload');
    console.log('-'.repeat(35));
    console.log('Best for: API integrations, structured data');
    
    try {
        const result = await makeJSONRequest('/minify', {
            code: sampleCode,
            level: 'advanced'
        });
        
        console.log('✅ Success!');
        console.log(`Original: ${result.stats.original_size} chars`);
        console.log(`Minified: ${result.stats.minified_size} chars`);
        console.log(`Compression: ${result.stats.compression_ratio}`);
        console.log(`Time: ${result.stats.processing_time_ms}ms`);
        console.log('Use case: Perfect for API calls, microservices');
        
    } catch (error) {
        console.log('❌ Error:', error.message);
    }
}

async function testMethod2_RawText() {
    console.log('\\n📄 Method 2: Raw Text Body');
    console.log('-'.repeat(35));
    console.log('Best for: Simple processing, minimal overhead');
    
    try {
        const result = await makeTextRequest('/minify/text?level=advanced', sampleCode);
        
        console.log('✅ Success!');
        console.log(`Compression: ${result.stats.compression_ratio}`);
        console.log(`Time: ${result.stats.processing_time_ms}ms`);
        console.log('Use case: CLI tools, build scripts, streaming');
        
    } catch (error) {
        console.log('❌ Error:', error.message);
    }
}

async function testMethod3_FileUpload() {
    console.log('\\n📁 Method 3: File Upload');
    console.log('-'.repeat(35));
    console.log('Best for: Web forms, browsers, file metadata');
    
    // Create temporary file
    const tempFile = '/tmp/sample.js';
    fs.writeFileSync(tempFile, sampleCode);
    
    try {
        // Create form data manually (simplified)
        const boundary = '----WebKitFormBoundary' + Math.random().toString(36);
        const formData = [
            `--${boundary}`,
            'Content-Disposition: form-data; name="jsfile"; filename="sample.js"',
            'Content-Type: text/javascript',
            '',
            sampleCode,
            `--${boundary}`,
            'Content-Disposition: form-data; name="level"',
            '',
            'advanced',
            `--${boundary}--`
        ].join('\\r\\n');
        
        const result = await makeFormRequest('/minify/upload', formData, boundary);
        
        console.log('✅ Success!');
        console.log(`File: ${result.file_info.original_name}`);
        console.log(`Size: ${result.file_info.size} bytes`);
        console.log(`Compression: ${result.stats.compression_ratio}`);
        console.log('Use case: Web uploads, browser forms, drag-and-drop');
        
        // Cleanup
        fs.unlinkSync(tempFile);
        
    } catch (error) {
        console.log('❌ Error:', error.message);
        if (fs.existsSync(tempFile)) fs.unlinkSync(tempFile);
    }
}

// Helper functions
function makeJSONRequest(path, data) {
    return new Promise((resolve, reject) => {
        const postData = JSON.stringify(data);
        
        const options = {
            hostname: 'localhost',
            port: 3002,
            path: path,
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Content-Length': Buffer.byteLength(postData)
            }
        };

        const req = http.request(options, (res) => {
            let responseData = '';
            res.on('data', chunk => responseData += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(responseData));
                } catch (e) {
                    reject(new Error('Invalid JSON response'));
                }
            });
        });

        req.on('error', reject);
        req.write(postData);
        req.end();
    });
}

function makeTextRequest(path, data) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: 'localhost',
            port: 3002,
            path: path,
            method: 'POST',
            headers: {
                'Content-Type': 'text/javascript',
                'Content-Length': Buffer.byteLength(data)
            }
        };

        const req = http.request(options, (res) => {
            let responseData = '';
            res.on('data', chunk => responseData += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(responseData));
                } catch (e) {
                    reject(new Error('Invalid response'));
                }
            });
        });

        req.on('error', reject);
        req.write(data);
        req.end();
    });
}

function makeFormRequest(path, formData, boundary) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: 'localhost',
            port: 3002,
            path: path,
            method: 'POST',
            headers: {
                'Content-Type': `multipart/form-data; boundary=${boundary}`,
                'Content-Length': Buffer.byteLength(formData)
            }
        };

        const req = http.request(options, (res) => {
            let responseData = '';
            res.on('data', chunk => responseData += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(responseData));
                } catch (e) {
                    reject(new Error('Invalid response'));
                }
            });
        });

        req.on('error', reject);
        req.write(formData);
        req.end();
    });
}

// Run all demos
async function runDemo() {
    try {
        await testMethod1_JSON();
        await testMethod2_RawText();
        await testMethod3_FileUpload();
        
        console.log('\\n🎯 Summary of Best Practices:');
        console.log('─'.repeat(40));
        console.log('📝 JSON Payload:    API integrations, structured data');
        console.log('📄 Raw Text:       CLI tools, simple processing');
        console.log('📁 File Upload:    Web forms, browser uploads');
        console.log('📁📁 Batch Upload:  Multiple files, build tools');
        console.log('🔗 URL Fetch:      External files, CDNs');
        console.log('🌊 Streaming:      Large files, memory efficiency');
        
        console.log('\\n💡 Recommendation:');
        console.log('For most use cases, start with JSON Payload method.');
        console.log('It offers the best balance of simplicity and features.');
        
    } catch (error) {
        console.error('Demo error:', error);
    }
}

runDemo();
