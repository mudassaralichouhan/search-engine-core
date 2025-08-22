const http = require('http');

// Test data
const testData = JSON.stringify({
  code: 'function test() { console.log("hello"); var unused = 123; /* comment */ }',
  level: 'advanced'
});

console.log('🧪 Testing JS Minifier Service');
console.log('Request payload:', testData);

const options = {
  hostname: 'localhost',
  port: 3002,
  path: '/minify',
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Content-Length': testData.length
  }
};

const req = http.request(options, (res) => {
  console.log('Response status:', res.statusCode);
  console.log('Response headers:', res.headers);
  
  let data = '';
  res.on('data', (chunk) => {
    data += chunk;
  });
  
  res.on('end', () => {
    try {
      const result = JSON.parse(data);
      console.log('✅ Success! Minification result:');
      console.log(JSON.stringify(result, null, 2));
    } catch (e) {
      console.log('❌ Error parsing response:', data);
    }
  });
});

req.on('error', (e) => {
  console.error('❌ Request error:', e);
});

req.write(testData);
req.end();
