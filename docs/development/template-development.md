# Template Development Guide

## 🏗️ Architecture

### Development vs Production

**Node.js Preview Server (Development):**

- Port 4000 - http://localhost:4000/
- Fast template rendering for development
- Test data and preview features
- Auto-reloads templates on changes
- Separate from production environment

**C++ Production Server:**

- Port 3000 - http://localhost:3000/crawl-request
- Production-ready with full Inja template engine
- Real localization data
- Optimized for performance
- Clean, simple responsibility

### Why This Architecture?

- ✅ **Separation of Concerns**: Development vs Production
- ✅ **Fast Development**: Node.js server for quick iterations
- ✅ **Production Quality**: C++ server with full template features
- ✅ **No Interference**: Development doesn't affect production
- ✅ **Simple Maintenance**: Each server has a clear purpose

## 🚀 Quick Start

### Using the Local Preview Server (Recommended for Development)

1. **Start the Node.js preview server:**

   ```bash
   cd development
   ./start-preview.sh
   # Or directly: node preview-server.js
   ```

2. **Open in browser:**
   - http://localhost:4000/ - Preview server homepage
   - http://localhost:4000/preview - Test data preview
   - http://localhost:4000/preview?lang=en - English preview

### Using the Production C++ Server

The production server serves the final template:

1. **Access production URLs:**
   - http://localhost:3000/crawl-request - Production English
   - http://localhost:3000/crawl-request?lang=fa - Production Persian (when available)

## 📁 File Structure

```
development/
├── README.md            # This file
├── preview-server.js    # Node.js preview server
└── start-preview.sh     # Startup script

templates/
└── crawl-request-full.inja  # Main template file

locales/
├── en.json             # English translations
├── test-data.json      # Test data for preview
└── fa.json             # Persian translations (future)

public/
├── css/
│   └── crawl-request-template.css
└── js/
    └── crawl-request-template.js
```

## 🎨 Development Workflow

### 1. Enable Syntax Highlighting

Install the **Better Jinja** extension in VSCode/Cursor for Inja syntax highlighting.

### 2. Edit Templates with Live Preview

1. Start the preview server
2. Edit `templates/crawl-request-full.inja`
3. Refresh browser to see changes instantly
4. The preview server reloads templates on each request

### 3. Test with Different Data

Edit `locales/test-data.json` to test different scenarios:

- Long text
- Empty fields
- Special characters
- RTL content

### 4. Preview Mode Features

When using the Node.js preview server, the template shows:

- Yellow banner at the top indicating preview mode
- Test data with [PREVIEW MODE] markers
- All features work exactly like production

## 🔧 Template Variables

### Available Variables in Template

```inja
{{ t.language.code }}        # Language code (en, fa, etc.)
{{ t.language.direction }}    # Text direction (ltr, rtl)
{{ t.meta.title }}           # Page title
{{ t.header.title }}         # Main heading
{{ t.form.submit_button }}   # Button text
{{ base_url }}               # Base URL for assets
{{ is_preview }}             # Boolean: true in Node.js preview server
```

### Conditional Rendering

```inja
{% if is_preview %}
  <div>Preview Mode Active</div>
{% endif %}

{% if t.language.direction == "rtl" %}
  <style>/* RTL styles */</style>
{% endif %}
```

## 🌍 Adding New Languages

1. Create `locales/[lang].json` (e.g., `locales/fa.json`)
2. Copy structure from `locales/en.json`
3. Translate all text values
4. Test with `?lang=[lang]` parameter in the Node.js preview server

## 🐛 Debugging

### Check Server Logs

```bash
docker-compose logs search-engine --tail=20
```

### Test Template Syntax

The preview server will show syntax errors if the template is invalid.

### Common Issues

1. **Template not updating:** The C++ server caches templates. Restart the container:

   ```bash
   docker-compose restart search-engine
   ```

2. **Preview banner not showing:** Ensure you're using the Node.js preview server (port 4000)

3. **CSS/JS not loading:** Check that files exist in `public/css/` and `public/js/`

## 🚀 Deployment

After testing in the Node.js preview server:

1. **Test in production mode:**

   ```bash
   curl http://localhost:3000/crawl-request
   ```

2. **Build and deploy:**
   ```bash
   cd build && make
   docker cp build/server core:/app/server
   docker-compose restart search-engine
   ```

## 💡 Tips

- Use test data that's clearly marked as "PREVIEW" or "TEST"
- Keep the Node.js preview server running during development
- Use browser DevTools to inspect rendered HTML
- Test both LTR and RTL languages
- Always verify in production mode before deploying
- The C++ server is for production only - keep it simple and clean

## 📚 Resources

- [Inja Documentation](https://github.com/pantor/inja)
- [Better Jinja Extension](https://marketplace.visualstudio.com/items?itemName=samuelcolvin.jinjahtml)
- [JSON Schema for Locales](https://json-schema.org/)
