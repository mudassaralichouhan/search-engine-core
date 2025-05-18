#include "Crawler.h"
#include "URLFrontier.h"
#include "RobotsTxtParser.h"
#include "PageFetcher.h"
#include "ContentParser.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

Crawler::Crawler(const CrawlConfig& config)
    : config(config)
    , isRunning(false) {
    urlFrontier = std::make_unique<URLFrontier>();
    robotsParser = std::make_unique<RobotsTxtParser>();
    pageFetcher = std::make_unique<PageFetcher>(
        config.userAgent,
        config.requestTimeout,
        config.followRedirects,
        config.maxRedirects
    );
    contentParser = std::make_unique<ContentParser>();
}

Crawler::~Crawler() {
    stop();
}

void Crawler::start() {
    if (isRunning) {
        return;
    }
    
    isRunning = true;
    std::thread crawlerThread(&Crawler::crawlLoop, this);
    crawlerThread.detach();
}

void Crawler::stop() {
    isRunning = false;
}

void Crawler::addSeedURL(const std::string& url) {
    urlFrontier->addURL(url);
}

std::vector<CrawlResult> Crawler::getResults() {
    std::lock_guard<std::mutex> lock(resultsMutex);
    return results;
}

void Crawler::crawlLoop() {
    while (isRunning) {
        // Get next URL to crawl
        std::string url = urlFrontier->getNextURL();
        if (url.empty()) {
            // No more URLs to crawl
            isRunning = false;
            break;
        }
        
        // Process the URL
        CrawlResult result = processURL(url);
        
        // Store the result
        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results.push_back(result);
        }
        
        // Check if we've reached the maximum number of pages
        if (results.size() >= config.maxPages) {
            isRunning = false;
            break;
        }
    }
}

CrawlResult Crawler::processURL(const std::string& url) {
    CrawlResult result;
    result.url = url;
    result.crawlTime = std::chrono::system_clock::now();
    
    // Check if URL is allowed by robots.txt
    if (config.respectRobotsTxt) {
        std::string domain = urlFrontier->extractDomain(url);
        if (!robotsParser->isAllowed(url, config.userAgent)) {
            result.success = false;
            result.errorMessage = "URL not allowed by robots.txt";
            return result;
        }
        
        // Respect crawl delay
        auto lastVisit = urlFrontier->getLastVisitTime(domain);
        auto crawlDelay = robotsParser->getCrawlDelay(domain, config.userAgent);
        auto timeSinceLastVisit = std::chrono::system_clock::now() - lastVisit;
        
        if (timeSinceLastVisit < crawlDelay) {
            std::this_thread::sleep_for(crawlDelay - timeSinceLastVisit);
        }
    }
    
    // Fetch the page
    auto fetchResult = pageFetcher->fetch(url);
    if (!fetchResult.success) {
        result.success = false;
        result.errorMessage = fetchResult.errorMessage;
        return result;
    }
    
    result.statusCode = fetchResult.statusCode;
    result.contentType = fetchResult.contentType;
    result.contentSize = fetchResult.content.size();
    
    // Parse the content if it's HTML
    if (fetchResult.contentType.find("text/html") != std::string::npos) {
        auto parsedContent = contentParser->parse(fetchResult.content, url);
        
        if (config.storeRawContent) {
            result.rawContent = fetchResult.content;
        }
        
        if (config.extractTextContent) {
            result.textContent = parsedContent.textContent;
        }
        
        result.title = parsedContent.title;
        result.metaDescription = parsedContent.metaDescription;
        
        // Add new URLs to the frontier
        extractAndAddURLs(fetchResult.content, url);
    }
    
    result.success = true;
    return result;
}

void Crawler::extractAndAddURLs(const std::string& content, const std::string& baseUrl) {
    auto links = contentParser->extractLinks(content, baseUrl);
    
    for (const auto& link : links) {
        // Check if URL is allowed by robots.txt
        if (!config.respectRobotsTxt || robotsParser->isAllowed(link, config.userAgent)) {
            urlFrontier->addURL(link);
        }
    }
} 