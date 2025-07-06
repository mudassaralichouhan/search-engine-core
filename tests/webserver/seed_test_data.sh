#!/bin/bash

# Seed test data for search endpoint integration tests
# This script creates a test index and populates it with sample documents

REDIS_CLI="redis-cli"
INDEX_NAME="${SEARCH_INDEX_NAME:-search_index}"

echo "Creating search index: $INDEX_NAME"

# Drop existing index if it exists (ignore errors)
$REDIS_CLI FT.DROPINDEX $INDEX_NAME 2>/dev/null || true

# Create the index with schema
$REDIS_CLI FT.CREATE $INDEX_NAME \
    ON HASH \
    PREFIX 1 "doc:" \
    SCHEMA \
    url TEXT WEIGHT 1.0 \
    title TEXT WEIGHT 5.0 \
    content TEXT WEIGHT 2.0 \
    domain TEXT \
    category TEXT \
    language TEXT \
    score NUMERIC \
    timestamp TEXT

echo "Seeding test documents..."

# Add sample documents
$REDIS_CLI HSET doc:1 \
    url "https://example.com/getting-started" \
    title "Getting Started with Search Engines" \
    content "This comprehensive guide will help you understand how modern search engines work, including crawling, indexing, and ranking algorithms. Learn the fundamentals of information retrieval and how to optimize your content for better search visibility." \
    domain "example.com" \
    category "tutorial" \
    language "en" \
    score "0.95" \
    timestamp "2024-01-15T10:00:00Z"

$REDIS_CLI HSET doc:2 \
    url "https://docs.example.com/api/search" \
    title "Search API Documentation" \
    content "Complete reference documentation for our search API endpoints. Learn how to integrate search functionality into your applications with examples in multiple programming languages. Covers authentication, query syntax, pagination, and filtering options." \
    domain "docs.example.com" \
    category "documentation" \
    language "en" \
    score "0.88" \
    timestamp "2024-01-14T15:30:00Z"

$REDIS_CLI HSET doc:3 \
    url "https://blog.techsite.org/advanced-search-techniques" \
    title "Advanced Search Techniques for Power Users" \
    content "Discover advanced search operators and techniques to find exactly what you're looking for. Master boolean operators, wildcards, fuzzy matching, and proximity searches. Tips and tricks for researchers and information professionals." \
    domain "blog.techsite.org" \
    category "blog" \
    language "en" \
    score "0.82" \
    timestamp "2024-01-13T09:15:00Z"

$REDIS_CLI HSET doc:4 \
    url "https://research.edu/papers/search-algorithms" \
    title "Comparative Analysis of Modern Search Algorithms" \
    content "Academic paper comparing the performance and accuracy of various search algorithms including BM25, TF-IDF, and neural ranking models. Includes benchmarks on standard test collections and practical recommendations for different use cases." \
    domain "research.edu" \
    category "research" \
    language "en" \
    score "0.91" \
    timestamp "2024-01-12T14:45:00Z"

echo "Test data seeded successfully!"
echo "Total documents indexed:"
$REDIS_CLI FT.INFO $INDEX_NAME | grep num_docs 