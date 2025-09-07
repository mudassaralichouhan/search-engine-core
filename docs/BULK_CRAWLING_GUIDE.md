# 🚀 **Bulk Domain Crawling & Email System Guide**

## **Overview**

This guide explains how to use the bulk domain crawling system to efficiently crawl 2000+ domains (5 pages each) and send email notifications to webmasters.

## **🏗️ Architecture**

```mermaid
graph TD
    A[Domain List CSV/API] --> B[DomainStorage MongoDB]
    B --> C[JobQueue Redis]
    C --> D[Worker Threads]
    D --> E[CrawlerManager]
    D --> F[EmailService]
    E --> G[Crawled Content]
    F --> H[Email Notifications]
    
    I[Monitoring Dashboard] --> B
    I --> C
    I --> J[Statistics API]
```

## **📋 Quick Start Guide**

### **1. Prepare Your Domain List**

Create a CSV file with your domains and webmaster emails:

```csv
domain,webmaster_email,max_pages
example.com,webmaster@example.com,5
test.org,admin@test.org,5
sample.net,contact@sample.net,5
```

### **2. Import Domains**

```bash
# Upload and import domains via API
curl -X POST "http://localhost:3000/api/v2/domains/import" \
  -H "Content-Type: text/csv" \
  --data-binary @domains.csv
```

### **3. Start Bulk Crawling**

```bash
# Start crawling all pending domains
curl -X POST "http://localhost:3000/api/v2/domains/crawl/bulk" \
  -H "Content-Type: application/json" \
  -d '{
    "maxConcurrent": 10,
    "emailNotifications": true,
    "crawlConfig": {
      "maxPages": 5,
      "politenessDelay": 1000,
      "respectRobotsTxt": true
    }
  }'
```

### **4. Monitor Progress**

```bash
# Check overall statistics
curl "http://localhost:3000/api/v2/domains/stats"

# Check job queue status
curl "http://localhost:3000/api/v2/domains/queue/stats"

# Get specific domain status
curl "http://localhost:3000/api/v2/domains/{domain-id}"
```

## **🔧 API Reference**

### **Domain Management**

#### **Add Single Domain**
```http
POST /api/v2/domains
Content-Type: application/json

{
  "domain": "example.com",
  "webmasterEmail": "webmaster@example.com",
  "maxPages": 5
}
```

#### **Add Multiple Domains**
```http
POST /api/v2/domains/bulk
Content-Type: application/json

{
  "domains": [
    {"domain": "example1.com", "webmasterEmail": "admin@example1.com", "maxPages": 5},
    {"domain": "example2.com", "webmasterEmail": "admin@example2.com", "maxPages": 5}
  ]
}
```

#### **Get Domain Status**
```http
GET /api/v2/domains/{domain-id}
```

Response:
```json
{
  "success": true,
  "data": {
    "id": "uuid-here",
    "domain": "example.com",
    "status": 2,
    "pagesCrawled": 5,
    "crawlSessionId": "session-id",
    "emailSentAt": "2025-01-14T10:30:00Z"
  }
}
```

### **Crawling Operations**

#### **Start Bulk Crawl**
```http
POST /api/v2/domains/crawl/bulk
Content-Type: application/json

{
  "maxConcurrent": 10,
  "emailNotifications": true,
  "crawlConfig": {
    "maxPages": 5,
    "maxDepth": 3,
    "politenessDelay": 1000,
    "respectRobotsTxt": true,
    "restrictToSeedDomain": true
  }
}
```

#### **Start Single Domain Crawl**
```http
POST /api/v2/domains/{domain-id}/crawl
Content-Type: application/json

{
  "emailNotification": true,
  "crawlConfig": {
    "maxPages": 5,
    "politenessDelay": 1000
  }
}
```

### **Monitoring & Statistics**

#### **Domain Statistics**
```http
GET /api/v2/domains/stats
```

Response:
```json
{
  "success": true,
  "data": {
    "totalDomains": 2000,
    "pendingDomains": 1500,
    "crawlingDomains": 25,
    "completedDomains": 450,
    "failedDomains": 25,
    "emailsSent": 430
  }
}
```

#### **Job Queue Statistics**
```http
GET /api/v2/domains/queue/stats
```

Response:
```json
{
  "success": true,
  "data": {
    "pending": 1500,
    "processing": 25,
    "completed": 900,
    "failed": 50,
    "total": 2475
  }
}
```

### **Email Operations**

#### **Send Test Email**
```http
POST /api/v2/domains/email/test
Content-Type: application/json

{
  "to": "test@example.com",
  "templateName": "webmaster_notification",
  "variables": {
    "domain": "example.com",
    "pagesCrawled": 5,
    "crawlDate": "2025-01-14"
  }
}
```

#### **Resend Failed Emails**
```http
POST /api/v2/domains/email/resend
```

## **⚙️ Configuration**

### **Environment Variables**

```bash
# Redis (Job Queue)
SEARCH_REDIS_URI=tcp://redis:6379

# MongoDB (Domain Storage)
MONGODB_URI=mongodb://admin:password123@mongodb:27017

# Email Configuration
EMAIL_USERNAME=your-smtp-username
EMAIL_PASSWORD=your-smtp-password
EMAIL_FROM=noreply@yourdomain.com

# Worker Configuration
JOB_WORKER_COUNT=4
MAX_CONCURRENT_CRAWLS=10
DEFAULT_CRAWL_TIMEOUT=600
```

### **Job Queue Configuration**

```cpp
// In your application startup
JobQueue jobQueue("tcp://redis:6379");

// Configure worker count based on your server capacity
int workerCount = std::getenv("JOB_WORKER_COUNT") ? 
    std::stoi(std::getenv("JOB_WORKER_COUNT")) : 4;

jobQueue.startWorkers(workerCount);
```

## **📊 Performance Recommendations**

### **For 2000 Domains (10,000 Pages)**

**Recommended Configuration:**
- **Workers**: 4-8 concurrent job workers
- **Crawl Delay**: 1-2 seconds between requests per domain
- **Max Concurrent Crawls**: 10-20 domains simultaneously
- **Batch Size**: Process 100-200 domains per batch
- **Total Time**: ~4-6 hours (depending on website response times)

**Resource Requirements:**
- **RAM**: 2-4 GB for the application
- **Redis**: 512 MB - 1 GB memory
- **MongoDB**: 1-2 GB storage + indexes
- **Network**: Stable connection with good bandwidth

### **Scaling for Larger Lists**

**For 10,000+ domains:**
1. **Horizontal Scaling**: Deploy multiple worker instances
2. **Redis Cluster**: Use Redis cluster for job distribution
3. **Load Balancing**: Distribute crawl load across regions
4. **Rate Limiting**: Implement per-domain rate limiting
5. **Retry Logic**: Enhanced retry with exponential backoff

## **🔍 Monitoring & Troubleshooting**

### **Real-time Monitoring**

```bash
# Watch job progress
watch -n 2 'curl -s "http://localhost:3000/api/v2/domains/queue/stats" | jq .'

# Monitor failed jobs
curl "http://localhost:3000/api/v2/domains?status=3&limit=10"

# Check system health
curl "http://localhost:3000/api/v2/domains/health"
```

### **Common Issues & Solutions**

**1. High Failure Rate**
- Check robots.txt compliance
- Increase politeness delay
- Verify domain accessibility
- Review error logs

**2. Slow Crawling**
- Increase worker count
- Optimize crawl configuration
- Check network latency
- Monitor server resources

**3. Email Delivery Issues**
- Verify SMTP credentials
- Check spam filters
- Validate email addresses
- Review email templates

**4. Memory Issues**
- Monitor Redis memory usage
- Implement job cleanup
- Optimize MongoDB queries
- Scale resources

### **Logging & Debugging**

Enable detailed logging:
```cpp
// Set log level for debugging
Logger::getInstance().init(LogLevel::DEBUG, true, "crawler.log");
```

Monitor specific components:
```bash
# Job queue logs
docker logs core | grep "JobQueue"

# Crawler logs
docker logs core | grep "Crawler"

# Email logs
docker logs core | grep "Email"
```

## **📈 Best Practices**

### **Crawling Ethics**
1. **Respect robots.txt** - Always enable robots.txt checking
2. **Politeness Delay** - Use 1-2 second delays between requests
3. **User Agent** - Use descriptive, contact-friendly user agent
4. **Rate Limiting** - Don't overwhelm target servers
5. **Time Limits** - Set reasonable crawl timeouts

### **Email Best Practices**
1. **Clear Subject Lines** - Be transparent about crawling activity
2. **Opt-out Options** - Provide easy unsubscribe mechanism
3. **Professional Templates** - Use well-formatted, informative emails
4. **Delivery Monitoring** - Track and retry failed deliveries
5. **Compliance** - Follow email marketing regulations

### **System Reliability**
1. **Error Handling** - Implement comprehensive error recovery
2. **Monitoring** - Set up alerts for system health
3. **Backups** - Regular database and configuration backups
4. **Testing** - Test with small batches before full deployment
5. **Graceful Shutdown** - Handle system restarts properly

## **🚀 Advanced Features**

### **Custom Crawl Configurations Per Domain**
```json
{
  "domain": "example.com",
  "webmasterEmail": "admin@example.com",
  "customConfig": {
    "maxPages": 10,
    "spaRendering": true,
    "customUserAgent": "SpecialBot/1.0"
  }
}
```

### **Webhook Notifications**
```json
{
  "webhookUrl": "https://your-api.com/crawl-complete",
  "events": ["domain_crawl_complete", "batch_complete"]
}
```

### **Scheduled Crawling**
```json
{
  "schedule": "0 2 * * 0",
  "recrawlInterval": "weekly",
  "maxStaleness": "30d"
}
```

This system provides a robust, scalable solution for bulk domain crawling with proper queue management, email notifications, and comprehensive monitoring capabilities.
