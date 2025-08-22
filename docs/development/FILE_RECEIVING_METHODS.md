# 🔄 Best Ways to Receive Files in HTTP APIs

This guide covers **6 different approaches** for receiving files in HTTP APIs, with practical examples using our JavaScript minifier service.

---

## **📋 Method Comparison**

| Method                      | Best For                      | Pros                                 | Cons                                | Max Size  |
| --------------------------- | ----------------------------- | ------------------------------------ | ----------------------------------- | --------- |
| **JSON Payload**            | API integrations, small files | Simple, structured, works everywhere | Base64 encoding overhead for binary | ~50MB     |
| **Raw Text/Binary**         | Simple text processing        | Minimal overhead, direct             | Limited metadata, single file only  | ~50MB     |
| **File Upload (multipart)** | Web forms, browsers           | Native browser support, metadata     | More complex parsing                | ~50MB     |
| **Multiple Files**          | Batch processing              | Handle many files at once            | Memory intensive                    | 10 files  |
| **URL Fetch**               | External file processing      | No upload needed, direct processing  | Network dependency, security risk   | Unlimited |
| **Streaming**               | Very large files              | Memory efficient, real-time          | Complex implementation              | Unlimited |

---

## **🛠️ Implementation Examples**

### **Method 1: JSON Payload** ⭐ _Most Common_

**Best for:** API integrations, microservices, small-medium files

```bash
# Example request
curl -X POST http://localhost:3002/minify/json \
  -H "Content-Type: application/json" \
  -d '{
    "code": "function test() { console.log(\"hello\"); }",
    "level": "advanced",
    "options": {"sourceMap": false}
  }'
```

**Express.js Implementation:**

```javascript
app.use(express.json({ limit: "50mb" }));

app.post("/minify/json", async (req, res) => {
  const { code, level, options } = req.body;
  // Process the code...
});
```

**✅ Pros:**

- Works with all HTTP clients
- Structured data with metadata
- Easy error handling
- Language agnostic

**❌ Cons:**

- JSON parsing overhead
- String escaping required

---

### **Method 2: Raw Text/Binary** ⚡ _Most Efficient_

**Best for:** Simple text processing, minimal overhead

```bash
# Example request
curl -X POST http://localhost:3002/minify/text?level=advanced \
  -H "Content-Type: text/javascript" \
  -d 'function test() { console.log("hello"); }'

# Return as minified JavaScript
curl -X POST http://localhost:3002/minify/text?level=advanced \
  -H "Content-Type: text/javascript" \
  -H "Accept: text/javascript" \
  -d 'function test() { console.log("hello"); }'
```

**Express.js Implementation:**

```javascript
app.use(express.text({ limit: "50mb", type: "text/javascript" }));
app.use(express.raw({ limit: "50mb", type: "application/octet-stream" }));

app.post("/minify/text", async (req, res) => {
  const code = req.body; // Direct access to raw content
  const level = req.query.level || "advanced";

  // Return based on Accept header
  if (req.get("Accept") === "text/javascript") {
    res.set("Content-Type", "text/javascript");
    res.send(minifiedCode);
  } else {
    res.json(result);
  }
});
```

**✅ Pros:**

- Minimal overhead
- Direct access to content
- Fast processing

**❌ Cons:**

- Limited metadata
- Single file only
- No structured response

---

### **Method 3: File Upload (Multipart)** 🌐 _Browser Friendly_

**Best for:** Web forms, browser uploads, file metadata needed

```html
<!-- HTML Form Example -->
<form action="/minify/upload" method="post" enctype="multipart/form-data">
  <input type="file" name="jsfile" accept=".js" />
  <select name="level">
    <option value="basic">Basic</option>
    <option value="advanced">Advanced</option>
    <option value="aggressive">Aggressive</option>
  </select>
  <button type="submit">Minify</button>
</form>
```

```bash
# cURL Example
curl -X POST http://localhost:3002/minify/upload \
  -F "jsfile=@/path/to/script.js" \
  -F "level=advanced"
```

**Express.js Implementation:**

```javascript
const multer = require("multer");
const upload = multer({
  storage: multer.memoryStorage(),
  limits: { fileSize: 50 * 1024 * 1024 }, // 50MB
  fileFilter: (req, file, cb) => {
    if (
      file.mimetype === "text/javascript" ||
      file.originalname.endsWith(".js")
    ) {
      cb(null, true);
    } else {
      cb(new Error("Only JavaScript files allowed!"), false);
    }
  },
});

app.post("/minify/upload", upload.single("jsfile"), async (req, res) => {
  const code = req.file.buffer.toString("utf8");
  const level = req.body.level;
  const filename = req.file.originalname;

  const result = await processMinification(code, level);
  result.file_info = {
    original_name: filename,
    size: req.file.size,
    mimetype: req.file.mimetype,
  };

  res.json(result);
});
```

**✅ Pros:**

- Native browser support
- File metadata included
- Widely supported
- Standard web approach

**❌ Cons:**

- More complex parsing
- Multipart overhead
- Requires multer or similar

---

### **Method 4: Multiple File Upload** 📁 _Batch Processing_

**Best for:** Processing multiple files simultaneously

```bash
# cURL Example
curl -X POST http://localhost:3002/minify/upload/batch \
  -F "jsfiles=@utils.js" \
  -F "jsfiles=@helpers.js" \
  -F "jsfiles=@main.js" \
  -F "level=advanced"
```

**Express.js Implementation:**

```javascript
app.post(
  "/minify/upload/batch",
  upload.array("jsfiles", 10),
  async (req, res) => {
    const level = req.body.level || "advanced";
    const results = [];

    for (const file of req.files) {
      try {
        const code = file.buffer.toString("utf8");
        const result = await processMinification(code, level);
        results.push({
          filename: file.originalname,
          success: true,
          ...result,
        });
      } catch (error) {
        results.push({
          filename: file.originalname,
          success: false,
          error: error.message,
        });
      }
    }

    res.json({ files: results });
  },
);
```

**✅ Pros:**

- Efficient batch processing
- Individual file results
- Good for build tools

**❌ Cons:**

- Memory intensive
- Complex error handling
- Limited by server memory

---

### **Method 5: URL-based Fetching** 🔗 _External Files_

**Best for:** Processing files from URLs, CDNs, external sources

```bash
# Example request
curl -X POST http://localhost:3002/minify/url \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://code.jquery.com/jquery-3.6.0.js",
    "level": "advanced"
  }'
```

**Express.js Implementation:**

```javascript
app.post("/minify/url", async (req, res) => {
  const { url, level = "advanced" } = req.body;

  // Validate URL
  try {
    new URL(url);
  } catch (e) {
    return res.status(400).json({ error: "Invalid URL" });
  }

  // Security: whitelist domains in production
  const allowedDomains = ["code.jquery.com", "cdn.jsdelivr.net"];
  const urlObj = new URL(url);
  if (!allowedDomains.includes(urlObj.hostname)) {
    return res.status(403).json({ error: "Domain not allowed" });
  }

  const response = await fetch(url, {
    timeout: 10000,
    headers: { "User-Agent": "JS-Minifier-Service/1.0" },
  });

  if (!response.ok) {
    return res.status(400).json({
      error: `HTTP ${response.status}: ${response.statusText}`,
    });
  }

  const code = await response.text();
  const result = await processMinification(code, level);

  result.source_info = {
    url: url,
    fetched_size: code.length,
    content_type: response.headers.get("content-type"),
  };

  res.json(result);
});
```

**✅ Pros:**

- No file upload needed
- Process remote files directly
- Good for build pipelines

**❌ Cons:**

- Network dependency
- Security risks (SSRF)
- External service reliability

---

### **Method 6: Streaming** 🌊 _Large Files_

**Best for:** Very large files, memory efficiency, real-time processing

```bash
# Example streaming request
cat large-file.js | curl -X POST http://localhost:3002/minify/stream?level=advanced \
  -H "Content-Type: text/javascript" \
  -H "Transfer-Encoding: chunked" \
  --data-binary @-
```

**Express.js Implementation:**

```javascript
app.post("/minify/stream", (req, res) => {
  let code = "";
  const level = req.query.level || "advanced";

  req.setEncoding("utf8");

  req.on("data", (chunk) => {
    code += chunk;
    console.log(`Received chunk: ${chunk.length} chars`);
  });

  req.on("end", async () => {
    try {
      const result = await processMinification(code, level);
      res.json(result);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  req.on("error", (error) => {
    res.status(500).json({ error: error.message });
  });
});
```

**✅ Pros:**

- Memory efficient
- Handle unlimited size files
- Real-time processing possible

**❌ Cons:**

- Complex implementation
- Error handling challenges
- Not suitable for all use cases

---

## **🎯 Recommendations by Use Case**

### **API Integration**

```bash
✅ Use: JSON Payload
🔧 Method: POST /minify/json
💡 Why: Structured, reliable, works everywhere
```

### **Web Application**

```bash
✅ Use: File Upload (Multipart)
🔧 Method: POST /minify/upload
💡 Why: Native browser support, user-friendly
```

### **Build Tools/CLI**

```bash
✅ Use: Raw Text or Streaming
🔧 Method: POST /minify/text or /minify/stream
💡 Why: Minimal overhead, pipe-friendly
```

### **Batch Processing**

```bash
✅ Use: Multiple File Upload
🔧 Method: POST /minify/upload/batch
💡 Why: Handle multiple files efficiently
```

### **CDN/External Processing**

```bash
✅ Use: URL Fetch
🔧 Method: POST /minify/url
💡 Why: No upload needed, direct processing
```

### **Very Large Files (>50MB)**

```bash
✅ Use: Streaming
🔧 Method: POST /minify/stream
💡 Why: Memory efficient, unlimited size
```

---

## **🛡️ Security Considerations**

1. **File Size Limits**: Always set reasonable limits
2. **File Type Validation**: Validate MIME types and extensions
3. **Content Scanning**: Scan for malicious code
4. **Rate Limiting**: Prevent abuse
5. **URL Whitelisting**: For URL fetch method
6. **Input Sanitization**: Validate all inputs
7. **Memory Management**: Monitor memory usage

---

## **⚡ Performance Tips**

1. **Use Raw Text** for simple cases (fastest)
2. **Stream large files** to avoid memory issues
3. **Implement caching** for repeated requests
4. **Use compression** (gzip) for responses
5. **Validate early** before processing
6. **Set appropriate timeouts**
7. **Monitor memory usage** with multiple files

---

## **🔧 Testing All Methods**

Run the comprehensive test:

```bash
node test-all-methods.js
```

This will test all 6 methods and show you the performance characteristics of each approach.

The **best method depends on your specific use case**, but for most applications, **JSON Payload** offers the best balance of simplicity, reliability, and features.
