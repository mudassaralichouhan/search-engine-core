const express = require('express');
const { minify } = require('terser');
const compression = require('compression');
const helmet = require('helmet');
const multer = require('multer');
const fs = require('fs').promises;

const app = express();
const PORT = process.env.PORT || 3002;

// Configure multer for file uploads
const storage = multer.memoryStorage();
const upload = multer({
    storage: storage,
    limits: {
        fileSize: 50 * 1024 * 1024, // 50MB limit
        files: 10 // Max 10 files at once
    },
    fileFilter: (req, file, cb) => {
        // Accept only JavaScript files
        if (file.mimetype === 'text/javascript' || 
            file.mimetype === 'application/javascript' ||
            file.originalname.endsWith('.js') ||
            file.originalname.endsWith('.mjs')) {
            cb(null, true);
        } else {
            cb(new Error('Only JavaScript files are allowed!'), false);
        }
    }
});

// Security and performance middleware
app.use(helmet());
app.use(compression());
app.use(express.json({ limit: '50mb' }));
app.use(express.text({ limit: '50mb', type: 'text/javascript' }));
app.use(express.raw({ limit: '50mb', type: 'application/octet-stream' }));

console.log('🗜️  Enhanced JS Minifier Service');
console.log('🔄 Supporting multiple file receiving methods:');
console.log('   • JSON payload (application/json)');
console.log('   • Raw text (text/javascript)');
console.log('   • File upload (multipart/form-data)');
console.log('   • URL-based fetching');
console.log('   • Streaming for large files');

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({
        status: 'healthy',
        service: 'js-minifier-enhanced',
        terser_version: require('terser').version,
        timestamp: new Date().toISOString(),
        supported_methods: [
            'JSON payload',
            'Raw text',
            'File upload',
            'URL fetch',
            'Streaming'
        ]
    });
});

// Method 1: JSON Payload (current approach - good for API integration)
app.post('/minify/json', async (req, res) => {
    try {
        console.log('📝 Processing JSON payload request');
        
        const {
            code,
            level = 'advanced',
            options = {}
        } = req.body;

        if (!code) {
            return res.status(400).json({
                error: 'No JavaScript code provided',
                message: 'Request body must contain the JavaScript code to minify'
            });
        }

        const result = await processMinification(code, level, options);
        res.json(result);

    } catch (error) {
        console.error('JSON minification error:', error);
        res.status(500).json({
            error: 'Minification failed',
            message: error.message
        });
    }
});

// Method 2: Raw Text (best for simple text files)
app.post('/minify/text', async (req, res) => {
    try {
        console.log('📄 Processing raw text request');
        
        const code = req.body;
        const level = req.query.level || 'advanced';

        if (!code || typeof code !== 'string') {
            return res.status(400).json({
                error: 'Invalid JavaScript code',
                message: 'Request body must contain JavaScript code as plain text'
            });
        }

        const result = await processMinification(code, level);
        
        // Return as JSON or plain text based on Accept header
        if (req.get('Accept') === 'text/javascript') {
            res.set('Content-Type', 'text/javascript');
            res.send(result.code);
        } else {
            res.json(result);
        }

    } catch (error) {
        console.error('Text minification error:', error);
        res.status(500).json({
            error: 'Minification failed',
            message: error.message
        });
    }
});

// Method 3: File Upload (best for actual file uploads from forms/browsers)
app.post('/minify/upload', upload.single('jsfile'), async (req, res) => {
    try {
        console.log('📁 Processing file upload request');
        
        if (!req.file) {
            return res.status(400).json({
                error: 'No file uploaded',
                message: 'Please upload a JavaScript file'
            });
        }

        const code = req.file.buffer.toString('utf8');
        const level = req.body.level || 'advanced';
        const filename = req.file.originalname;

        console.log(`Processing uploaded file: ${filename} (${code.length} chars)`);

        const result = await processMinification(code, level);
        
        // Add file info to result
        result.file_info = {
            original_name: filename,
            size: req.file.size,
            mimetype: req.file.mimetype
        };

        res.json(result);

    } catch (error) {
        console.error('Upload minification error:', error);
        res.status(500).json({
            error: 'File processing failed',
            message: error.message
        });
    }
});

// Method 4: Multiple File Upload (best for batch processing)
app.post('/minify/upload/batch', upload.array('jsfiles', 10), async (req, res) => {
    try {
        console.log('📁📁 Processing multiple file upload request');
        
        if (!req.files || req.files.length === 0) {
            return res.status(400).json({
                error: 'No files uploaded',
                message: 'Please upload JavaScript files'
            });
        }

        const level = req.body.level || 'advanced';
        const results = [];

        for (const file of req.files) {
            try {
                const code = file.buffer.toString('utf8');
                console.log(`Processing: ${file.originalname} (${code.length} chars)`);
                
                const result = await processMinification(code, level);
                results.push({
                    filename: file.originalname,
                    success: true,
                    ...result,
                    file_info: {
                        original_name: file.originalname,
                        size: file.size,
                        mimetype: file.mimetype
                    }
                });
            } catch (error) {
                results.push({
                    filename: file.originalname,
                    success: false,
                    error: error.message
                });
            }
        }

        res.json({
            files: results,
            summary: {
                total_files: req.files.length,
                successful: results.filter(r => r.success).length,
                failed: results.filter(r => !r.success).length
            }
        });

    } catch (error) {
        console.error('Batch upload error:', error);
        res.status(500).json({
            error: 'Batch processing failed',
            message: error.message
        });
    }
});

// Method 5: URL-based fetching (best for processing files from URLs)
app.post('/minify/url', async (req, res) => {
    try {
        console.log('🔗 Processing URL fetch request');
        
        const { url, level = 'advanced', options = {} } = req.body;

        if (!url) {
            return res.status(400).json({
                error: 'No URL provided',
                message: 'Please provide a URL to fetch JavaScript from'
            });
        }

        // Validate URL
        try {
            new URL(url);
        } catch (e) {
            return res.status(400).json({
                error: 'Invalid URL',
                message: 'Please provide a valid URL'
            });
        }

        console.log(`Fetching JavaScript from: ${url}`);

        // Fetch the JavaScript file
        const response = await fetch(url, {
            timeout: 10000, // 10 second timeout
            headers: {
                'User-Agent': 'JS-Minifier-Service/1.0'
            }
        });

        if (!response.ok) {
            return res.status(400).json({
                error: 'Failed to fetch URL',
                message: `HTTP ${response.status}: ${response.statusText}`
            });
        }

        const code = await response.text();
        console.log(`Fetched ${code.length} characters from ${url}`);

        const result = await processMinification(code, level, options);
        
        // Add URL info to result
        result.source_info = {
            url: url,
            fetched_size: code.length,
            content_type: response.headers.get('content-type')
        };

        res.json(result);

    } catch (error) {
        console.error('URL fetch error:', error);
        res.status(500).json({
            error: 'URL processing failed',
            message: error.message
        });
    }
});

// Method 6: Streaming (best for very large files)
app.post('/minify/stream', (req, res) => {
    console.log('🌊 Processing streaming request');
    
    let code = '';
    const level = req.query.level || 'advanced';

    req.setEncoding('utf8');

    req.on('data', (chunk) => {
        code += chunk;
        console.log(`Received chunk: ${chunk.length} chars (total: ${code.length})`);
    });

    req.on('end', async () => {
        try {
            console.log(`Stream complete: ${code.length} total characters`);
            
            if (!code) {
                return res.status(400).json({
                    error: 'No code received',
                    message: 'Stream was empty'
                });
            }

            const result = await processMinification(code, level);
            res.json(result);

        } catch (error) {
            console.error('Stream processing error:', error);
            res.status(500).json({
                error: 'Stream processing failed',
                message: error.message
            });
        }
    });

    req.on('error', (error) => {
        console.error('Stream error:', error);
        res.status(500).json({
            error: 'Stream error',
            message: error.message
        });
    });
});

// Helper function for consistent minification processing
async function processMinification(code, level = 'advanced', customOptions = {}) {
    const startTime = Date.now();
    
    const defaultOptions = getMinificationOptions(level);
    const terserOptions = { ...defaultOptions, ...customOptions };

    const result = await minify(code, terserOptions);

    if (result.error) {
        throw new Error(result.error.message || result.error);
    }

    const processingTime = Date.now() - startTime;
    const originalSize = code.length;
    const minifiedSize = result.code ? result.code.length : 0;
    const compressionRatio = originalSize > 0 ? 
        ((originalSize - minifiedSize) / originalSize * 100).toFixed(2) + '%' : '0%';

    return {
        code: result.code || code,
        map: result.map || null,
        stats: {
            original_size: originalSize,
            minified_size: minifiedSize,
            compression_ratio: compressionRatio,
            processing_time_ms: processingTime,
            level: level
        }
    };
}

function getMinificationOptions(level) {
    const baseOptions = {
        ecma: 2020,
        sourceMap: false,
        format: {
            comments: false,
            beautify: false,
            semicolons: true
        }
    };

    switch (level.toLowerCase()) {
        case 'none':
            return { ...baseOptions, compress: false, mangle: false };
        case 'basic':
            return {
                ...baseOptions,
                compress: { drop_console: false, drop_debugger: false, passes: 1 },
                mangle: false
            };
        case 'advanced':
            return {
                ...baseOptions,
                compress: {
                    drop_console: false, drop_debugger: true, pure_funcs: ['console.log'],
                    passes: 2, unsafe: false
                },
                mangle: { toplevel: false, eval: false, keep_fnames: false }
            };
        case 'aggressive':
            return {
                ...baseOptions,
                compress: {
                    drop_console: true, drop_debugger: true, passes: 3,
                    unsafe: true, unsafe_comps: true, toplevel: true
                },
                mangle: { toplevel: true, eval: true }
            };
        default:
            return getMinificationOptions('advanced');
    }
}

// Configuration endpoint
app.get('/config', (req, res) => {
    res.json({
        available_levels: ['none', 'basic', 'advanced', 'aggressive'],
        default_level: 'advanced',
        supported_methods: {
            'POST /minify/json': 'JSON payload with code property',
            'POST /minify/text': 'Raw JavaScript as request body',
            'POST /minify/upload': 'Single file upload (multipart/form-data)',
            'POST /minify/upload/batch': 'Multiple file upload',
            'POST /minify/url': 'Fetch JavaScript from URL',
            'POST /minify/stream': 'Streaming for large files'
        },
        max_file_size: '50MB',
        max_files_per_request: 10,
        terser_version: require('terser').version
    });
});

// Error handling middleware
app.use((error, req, res, next) => {
    console.error('Unhandled error:', error);
    
    if (error instanceof multer.MulterError) {
        if (error.code === 'LIMIT_FILE_SIZE') {
            return res.status(413).json({
                error: 'File too large',
                message: 'Maximum file size is 50MB'
            });
        }
        if (error.code === 'LIMIT_FILE_COUNT') {
            return res.status(413).json({
                error: 'Too many files',
                message: 'Maximum 10 files per request'
            });
        }
    }
    
    res.status(500).json({
        error: 'Internal server error',
        message: error.message,
        timestamp: new Date().toISOString()
    });
});

app.listen(PORT, () => {
    console.log(`🗜️  Enhanced JS Minifier Service running on port ${PORT}`);
    console.log(`📊 Health check: http://localhost:${PORT}/health`);
    console.log(`⚙️  Configuration: http://localhost:${PORT}/config`);
    console.log(`🚀 Ready to accept files via multiple methods!`);
});

// Graceful shutdown
process.on('SIGTERM', () => {
    console.log('Received SIGTERM, shutting down gracefully');
    process.exit(0);
});

process.on('SIGINT', () => {
    console.log('Received SIGINT, shutting down gracefully');  
    process.exit(0);
});
