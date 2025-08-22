const http = require('http');
const https = require('https');
const fs = require('fs');
const FormData = require('form-data');

// Sample JavaScript code for testing
const testCode = `
// Sample JavaScript for testing different receiving methods
function calculateTotalPrice(items, tax = 0.08) {
    let subtotal = 0;
    
    // Calculate subtotal
    for (const item of items) {
        if (item.price && item.quantity) {
            subtotal += item.price * item.quantity;
            console.log('Processing item:', item.name);
        }
    }
    
    // Apply tax
    const taxAmount = subtotal * tax;
    const total = subtotal + taxAmount;
    
    console.log('Subtotal:', subtotal);
    console.log('Tax:', taxAmount);
    console.log('Total:', total);
    
    return {
        subtotal: subtotal,
        tax: taxAmount,
        total: total
    };
}

// Unused code for testing dead code elimination
const unusedVariable = 'This should be removed';
debugger; // Should be removed in production
`;

console.log('🧪 Testing All File Receiving Methods');
console.log('=' .repeat(50));

async function testMethod1_JSON() {
    console.log('\\n📝 Method 1: JSON Payload');
    console.log('-'.repeat(30));
    
    const payload = JSON.stringify({
        code: testCode,
        level: 'advanced'
    });

    try {
        const result = await makeRequest('/minify/json', 'POST', payload, {
            'Content-Type': 'application/json'
        });
        
        console.log('✅ Success!');
        console.log('Compression:', result.stats.compression_ratio);
        console.log('Processing time:', result.stats.processing_time_ms + 'ms');
        
    } catch (error) {
        console.log('❌ Error:', error.message);
    }
}

async function testMethod2_RawText() {
    console.log('\\n📄 Method 2: Raw Text');
    console.log('-'.repeat(30));
    
    try {
        const result = await makeRequest('/minify/text?level=advanced', 'POST', testCode, {
            'Content-Type': 'text/javascript'
        });
        
        console.log('✅ Success!');
        console.log('Compression:', result.stats.compression_ratio);
        
    } catch (error) {
        console.log('❌ Error:', error.message);
    }
}

async function testMethod3_FileUpload() {
    console.log('\\n📁 Method 3: File Upload');
    console.log('-'.repeat(30));
    
    // Create a temporary file
    const tempFile = '/tmp/test.js';
    fs.writeFileSync(tempFile, testCode);
    
    try {
        const form = new FormData();
        form.append('jsfile', fs.createReadStream(tempFile));
        form.append('level', 'advanced');
        
        const result = await makeFormRequest('/minify/upload', form);
        
        console.log('✅ Success!');
        console.log('File:', result.file_info.original_name);
        console.log('Compression:', result.stats.compression_ratio);
        
        // Clean up
        fs.unlinkSync(tempFile);
        
    } catch (error) {
        console.log('❌ Error:', error.message);
        // Clean up on error
        if (fs.existsSync(tempFile)) fs.unlinkSync(tempFile);
    }
}

async function testMethod4_MultipleFiles() {
    console.log('\\n📁📁 Method 4: Multiple File Upload');
    console.log('-'.repeat(30));
    
    // Create temporary files
    const files = [
        { path: '/tmp/utils.js', content: 'function debounce(func, wait) { let timeout; return (...args) => { clearTimeout(timeout); timeout = setTimeout(() => func(...args), wait); }; }' },
        { path: '/tmp/helpers.js', content: 'function formatCurrency(amount) { return new Intl.NumberFormat("en-US", { style: "currency", currency: "USD" }).format(amount); }' }
    ];
    
    files.forEach(file => fs.writeFileSync(file.path, file.content));
    
    try {
        const form = new FormData();
        files.forEach(file => {
            form.append('jsfiles', fs.createReadStream(file.path));
        });
        form.append('level', 'advanced');
        
        const result = await makeFormRequest('/minify/upload/batch', form);
        
        console.log('✅ Success!');
        console.log('Files processed:', result.summary.total_files);
        console.log('Successful:', result.summary.successful);
        
        // Clean up
        files.forEach(file => fs.unlinkSync(file.path));
        
    } catch (error) {
        console.log('❌ Error:', error.message);
        // Clean up on error
        files.forEach(file => {
            if (fs.existsSync(file.path)) fs.unlinkSync(file.path);
        });
    }
}

async function testMethod5_URLFetch() {
    console.log('\\n🔗 Method 5: URL Fetch');
    console.log('-'.repeat(30));
    
    const payload = JSON.stringify({
        url: 'https://code.jquery.com/jquery-3.6.0.min.js',
        level: 'basic' // Already minified, so use basic
    });

    try {
        const result = await makeRequest('/minify/url', 'POST', payload, {
            'Content-Type': 'application/json'
        });
        
        console.log('✅ Success!');
        console.log('Source URL:', result.source_info.url);
        console.log('Fetched size:', result.source_info.fetched_size);
        console.log('Final compression:', result.stats.compression_ratio);
        
    } catch (error) {
        console.log('❌ Error (expected for network):', error.message);
    }
}

async function testMethod6_Streaming() {
    console.log('\\n🌊 Method 6: Streaming');
    console.log('-'.repeat(30));
    
    try {
        const result = await makeStreamRequest('/minify/stream?level=advanced', testCode);
        
        console.log('✅ Success!');
        console.log('Compression:', result.stats.compression_ratio);
        
    } catch (error) {
        console.log('❌ Error:', error.message);
    }
}

// Helper functions
function makeRequest(path, method, data, headers = {}) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: 'localhost',
            port: 3002,
            path: path,
            method: method,
            headers: {
                'Content-Length': Buffer.byteLength(data),
                ...headers
            }
        };

        const req = http.request(options, (res) => {
            let responseData = '';
            res.on('data', chunk => responseData += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(responseData));
                } catch (e) {
                    resolve(responseData);
                }
            });
        });

        req.on('error', reject);
        req.write(data);
        req.end();
    });
}

function makeFormRequest(path, form) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: 'localhost',
            port: 3002,
            path: path,
            method: 'POST',
            headers: form.getHeaders()
        };

        const req = http.request(options, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(data));
                } catch (e) {
                    reject(new Error('Invalid JSON response'));
                }
            });
        });

        req.on('error', reject);
        form.pipe(req);
    });
}

function makeStreamRequest(path, data) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: 'localhost',
            port: 3002,
            path: path,
            method: 'POST',
            headers: {
                'Content-Type': 'text/javascript',
                'Transfer-Encoding': 'chunked'
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
        
        // Send data in chunks to simulate streaming
        const chunks = data.match(/.{1,100}/g) || [];
        let i = 0;
        
        const sendChunk = () => {
            if (i < chunks.length) {
                req.write(chunks[i]);
                i++;
                setTimeout(sendChunk, 10); // Small delay between chunks
            } else {
                req.end();
            }
        };
        
        sendChunk();
    });
}

// Run all tests
async function runAllTests() {
    try {
        await testMethod1_JSON();
        await testMethod2_RawText();
        await testMethod3_FileUpload();
        await testMethod4_MultipleFiles();
        await testMethod5_URLFetch();
        await testMethod6_Streaming();
        
        console.log('\\n🎉 All file receiving method tests completed!');
        console.log('\\n📊 Summary of Best Practices:');
        console.log('   • JSON Payload: Best for API integrations');
        console.log('   • Raw Text: Best for simple text processing');
        console.log('   • File Upload: Best for web forms and browsers');
        console.log('   • Multiple Files: Best for batch processing');
        console.log('   • URL Fetch: Best for processing external files');
        console.log('   • Streaming: Best for very large files');
        
    } catch (error) {
        console.error('Test suite error:', error);
    }
}

runAllTests();
