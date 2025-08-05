#!/usr/bin/env node

/**
 * Local Preview Server for Inja Template Development
 * 
 * Usage:
 *   node preview-server.js
 *   
 * Then visit:
 *   http://localhost:4000/preview - English preview
 *   http://localhost:4000/preview?lang=fa - Persian preview
 *   http://localhost:4000/preview?lang=test - Test data preview
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const url = require('url');

const PORT = 4000;
const BASE_DIR = path.resolve(__dirname, '..');

// Simple Inja template renderer (basic implementation)
function renderTemplate(templatePath, data) {
    let template = fs.readFileSync(templatePath, 'utf8');
    
    // Replace {{ variable }} patterns
    template = template.replace(/\{\{\s*([\w.]+)\s*\}\}/g, (match, key) => {
        const keys = key.split('.');
        let value = data;
        for (const k of keys) {
            if (value && typeof value === 'object') {
                value = value[k];
            } else {
                value = undefined;
                break;
            }
        }
        return value !== undefined ? value : match;
    });
    
    // Handle {% if %} blocks with string comparison
    template = template.replace(/\{%\s*if\s+([\w.]+)\s*==\s*"([^"]+)"\s*%\}([\s\S]*?)\{%\s*endif\s*%\}/g, 
        (match, variable, compareValue, content) => {
            const keys = variable.split('.');
            let value = data;
            for (const k of keys) {
                if (value && typeof value === 'object') {
                    value = value[k];
                } else {
                    value = undefined;
                    break;
                }
            }
            return value === compareValue ? content : '';
        });
    
    // Handle {% if %} blocks with boolean checks
    template = template.replace(/\{%\s*if\s+([\w.]+)\s*%\}([\s\S]*?)\{%\s*endif\s*%\}/g,
        (match, variable, content) => {
            const keys = variable.split('.');
            let value = data;
            for (const k of keys) {
                if (value && typeof value === 'object') {
                    value = value[k];
                } else {
                    value = undefined;
                    break;
                }
            }
            return value ? content : '';
        });
    
    return template;
}

// Load locale data
function loadLocaleData(lang) {
    const localePath = path.join(BASE_DIR, 'locales', `${lang}.json`);
    
    // Special handling for test data
    if (lang === 'test') {
        const testPath = path.join(BASE_DIR, 'locales', 'test-data.json');
        if (fs.existsSync(testPath)) {
            return JSON.parse(fs.readFileSync(testPath, 'utf8'));
        }
    }
    
    if (fs.existsSync(localePath)) {
        return JSON.parse(fs.readFileSync(localePath, 'utf8'));
    }
    
    // Fallback to English
    const enPath = path.join(BASE_DIR, 'locales', 'en.json');
    if (fs.existsSync(enPath)) {
        return JSON.parse(fs.readFileSync(enPath, 'utf8'));
    }
    
    return null;
}

// Create server
const server = http.createServer((req, res) => {
    const parsedUrl = url.parse(req.url, true);
    const pathname = parsedUrl.pathname;
    
    // Handle preview route
    if (pathname === '/preview' || pathname === '/preview/') {
        const lang = parsedUrl.query.lang || 'test';
        const localeData = loadLocaleData(lang);
        
        if (!localeData) {
            res.writeHead(404, { 'Content-Type': 'text/html' });
            res.end('<h1>Locale data not found</h1>');
            return;
        }
        
        const templatePath = path.join(BASE_DIR, 'templates', 'crawl-request-full.inja');
        
        if (!fs.existsSync(templatePath)) {
            res.writeHead(404, { 'Content-Type': 'text/html' });
            res.end('<h1>Template not found</h1>');
            return;
        }
        
        const templateData = {
            t: localeData,
            base_url: `http://localhost:${PORT}`,
            is_preview: true
        };
        
        // Add preview banner for development
        let html = renderTemplate(templatePath, templateData);
        if (html) {
            const previewBanner = `
    <div style="background: #ffc107; color: #000; padding: 10px; text-align: center; font-weight: bold; position: fixed; top: 0; left: 0; right: 0; z-index: 9999;">
        🔧 PREVIEW MODE - This is test data for development
    </div>
    <div style="height: 40px;"></div>
`;
            html = html.replace('<body>', '<body>' + previewBanner);
        }
        
        try {
            res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
            res.end(html);
        } catch (error) {
            res.writeHead(500, { 'Content-Type': 'text/html' });
            res.end(`<h1>Template Rendering Error</h1><pre>${error.message}</pre>`);
        }
        
    // Handle static assets (CSS, JS)
    } else if (pathname.startsWith('/css/') || pathname.startsWith('/js/')) {
        const filePath = path.join(BASE_DIR, 'public', pathname.slice(1));
        
        if (fs.existsSync(filePath)) {
            const ext = path.extname(filePath);
            const contentType = ext === '.css' ? 'text/css' : 'application/javascript';
            
            res.writeHead(200, { 'Content-Type': contentType });
            fs.createReadStream(filePath).pipe(res);
        } else {
            res.writeHead(404);
            res.end('Not found');
        }
        
    // Handle root
    } else if (pathname === '/') {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.end(`
            <!DOCTYPE html>
            <html>
            <head>
                <title>Template Preview Server</title>
                <style>
                    body { font-family: Arial, sans-serif; padding: 40px; background: #f5f5f5; }
                    .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
                    h1 { color: #333; }
                    .links { margin-top: 30px; }
                    .link-item { margin: 15px 0; padding: 15px; background: #f8f9fa; border-radius: 5px; }
                    a { color: #007bff; text-decoration: none; font-size: 18px; }
                    a:hover { text-decoration: underline; }
                    .desc { color: #666; margin-top: 5px; font-size: 14px; }
                    .status { display: inline-block; padding: 3px 8px; border-radius: 3px; font-size: 12px; margin-left: 10px; }
                    .status.preview { background: #ffc107; color: #000; }
                    .status.test { background: #28a745; color: white; }
                </style>
            </head>
            <body>
                <div class="container">
                    <h1>🚀 Template Preview Server</h1>
                    <p>Development server for Inja template preview and testing</p>
                    
                    <div class="links">
                        <div class="link-item">
                            <a href="/preview">📝 Preview with Test Data</a>
                            <span class="status test">TEST</span>
                            <div class="desc">View template with test data (includes PREVIEW markers)</div>
                        </div>
                        
                        <div class="link-item">
                            <a href="/preview?lang=en">🇺🇸 English Preview</a>
                            <span class="status preview">PREVIEW</span>
                            <div class="desc">View template with English localization</div>
                        </div>
                        
                        <div class="link-item">
                            <a href="/preview?lang=fa">🇮🇷 Persian Preview</a>
                            <span class="status preview">PREVIEW</span>
                            <div class="desc">View template with Persian localization (if available)</div>
                        </div>
                    </div>
                    
                    <div style="margin-top: 40px; padding-top: 20px; border-top: 1px solid #e9ecef;">
                        <h3>🛠️ Development Tips:</h3>
                        <ul style="color: #666; line-height: 1.8;">
                            <li>Edit <code>templates/crawl-request-full.inja</code> to modify the template</li>
                            <li>Edit <code>locales/test-data.json</code> to change test content</li>
                            <li>This server auto-reloads templates on each request</li>
                            <li>Use <code>?lang=XX</code> to test different languages</li>
                            <li>CSS and JS files are served from <code>public/</code> directory</li>
                        </ul>
                    </div>
                </div>
            </body>
            </html>
        `);
    } else {
        res.writeHead(404);
        res.end('Not found');
    }
});

// Start server
server.listen(PORT, () => {
    console.log(`
╔════════════════════════════════════════════════════╗
║      🚀 Template Preview Server Started!           ║
╠════════════════════════════════════════════════════╣
║                                                    ║
║  Preview URLs:                                     ║
║  • http://localhost:${PORT}/                       ║
║  • http://localhost:${PORT}/preview (test data)    ║
║  • http://localhost:${PORT}/preview?lang=en        ║
║  • http://localhost:${PORT}/preview?lang=fa        ║
║                                                    ║
║  Press Ctrl+C to stop the server                  ║
╚════════════════════════════════════════════════════╝
    `);
});

// Handle graceful shutdown
process.on('SIGINT', () => {
    console.log('\n👋 Shutting down preview server...');
    server.close(() => {
        console.log('✅ Server stopped');
        process.exit(0);
    });
});