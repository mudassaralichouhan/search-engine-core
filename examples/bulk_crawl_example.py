#!/usr/bin/env python3
"""
Bulk Domain Crawling Example Script

This script demonstrates how to use the bulk crawling API to:
1. Import 2000 domains with webmaster emails
2. Start bulk crawling (5 pages per domain)
3. Monitor progress and send email notifications

Usage:
    python3 bulk_crawl_example.py domains.csv
"""

import csv
import json
import time
import requests
import argparse
from typing import List, Dict, Optional

class BulkCrawler:
    def __init__(self, base_url: str = "http://localhost:3000"):
        self.base_url = base_url
        self.api_base = f"{base_url}/api/v2"
        
    def import_domains_from_csv(self, csv_file: str) -> bool:
        """Import domains from CSV file"""
        print(f"📂 Reading domains from {csv_file}...")
        
        domains = []
        with open(csv_file, 'r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                domains.append({
                    "domain": row['domain'],
                    "webmasterEmail": row['webmaster_email'],
                    "maxPages": int(row.get('max_pages', 5))
                })
        
        print(f"📝 Found {len(domains)} domains to import")
        
        # Import in batches of 100
        batch_size = 100
        imported_count = 0
        
        for i in range(0, len(domains), batch_size):
            batch = domains[i:i + batch_size]
            
            response = requests.post(
                f"{self.api_base}/domains/bulk",
                json={"domains": batch},
                headers={"Content-Type": "application/json"}
            )
            
            if response.status_code == 200:
                result = response.json()
                if result.get('success'):
                    imported_count += len(batch)
                    print(f"✅ Imported batch {i//batch_size + 1}: {len(batch)} domains")
                else:
                    print(f"❌ Failed to import batch {i//batch_size + 1}: {result.get('message')}")
                    return False
            else:
                print(f"❌ HTTP Error {response.status_code} importing batch {i//batch_size + 1}")
                return False
            
            # Small delay between batches
            time.sleep(0.5)
        
        print(f"🎉 Successfully imported {imported_count} domains")
        return True
    
    def start_bulk_crawl(self, max_concurrent: int = 10) -> Optional[str]:
        """Start bulk crawling operation"""
        print(f"🚀 Starting bulk crawl with {max_concurrent} concurrent workers...")
        
        crawl_config = {
            "maxConcurrent": max_concurrent,
            "emailNotifications": True,
            "crawlConfig": {
                "maxPages": 5,
                "maxDepth": 3,
                "politenessDelay": 1000,  # 1 second between requests
                "respectRobotsTxt": True,
                "restrictToSeedDomain": True,
                "requestTimeout": 30000  # 30 second timeout
            }
        }
        
        response = requests.post(
            f"{self.api_base}/domains/crawl/bulk",
            json=crawl_config,
            headers={"Content-Type": "application/json"}
        )
        
        if response.status_code == 200:
            result = response.json()
            if result.get('success'):
                batch_id = result.get('data', {}).get('batchId')
                print(f"✅ Bulk crawl started successfully (Batch ID: {batch_id})")
                return batch_id
            else:
                print(f"❌ Failed to start bulk crawl: {result.get('message')}")
        else:
            print(f"❌ HTTP Error {response.status_code} starting bulk crawl")
        
        return None
    
    def get_stats(self) -> Dict:
        """Get domain and queue statistics"""
        try:
            # Get domain stats
            domain_response = requests.get(f"{self.api_base}/domains/stats")
            queue_response = requests.get(f"{self.api_base}/domains/queue/stats")
            
            domain_stats = domain_response.json().get('data', {}) if domain_response.status_code == 200 else {}
            queue_stats = queue_response.json().get('data', {}) if queue_response.status_code == 200 else {}
            
            return {
                "domains": domain_stats,
                "queue": queue_stats
            }
        except Exception as e:
            print(f"❌ Error getting stats: {e}")
            return {"domains": {}, "queue": {}}
    
    def monitor_progress(self, check_interval: int = 30):
        """Monitor crawling progress with real-time updates"""
        print(f"📊 Starting monitoring (checking every {check_interval} seconds)...")
        print("📈 Progress Dashboard:")
        print("=" * 80)
        
        start_time = time.time()
        last_completed = 0
        
        while True:
            try:
                stats = self.get_stats()
                domain_stats = stats.get('domains', {})
                queue_stats = stats.get('queue', {})
                
                # Calculate progress
                total_domains = domain_stats.get('totalDomains', 0)
                completed = domain_stats.get('completedDomains', 0)
                failed = domain_stats.get('failedDomains', 0)
                crawling = domain_stats.get('crawlingDomains', 0)
                pending = domain_stats.get('pendingDomains', 0)
                emails_sent = domain_stats.get('emailsSent', 0)
                
                # Queue stats
                queue_pending = queue_stats.get('pending', 0)
                queue_processing = queue_stats.get('processing', 0)
                queue_completed = queue_stats.get('completed', 0)
                queue_failed = queue_stats.get('failed', 0)
                
                # Calculate rates
                elapsed_time = time.time() - start_time
                completion_rate = (completed - last_completed) / check_interval if elapsed_time > check_interval else 0
                eta_seconds = (pending / completion_rate) if completion_rate > 0 else 0
                
                # Clear screen and show updated stats
                print("\033[2J\033[H")  # Clear screen
                print("🔍 **BULK CRAWLING DASHBOARD** 🔍")
                print("=" * 80)
                print(f"⏰ Runtime: {elapsed_time/60:.1f} minutes")
                print(f"📊 Progress: {completed}/{total_domains} domains ({(completed/max(total_domains,1)*100):.1f}%)")
                print(f"🏃 Rate: {completion_rate*60:.1f} domains/minute")
                print(f"⏳ ETA: {eta_seconds/60:.1f} minutes remaining")
                print()
                print("📋 **DOMAIN STATUS**")
                print(f"  ✅ Completed: {completed:,}")
                print(f"  ⏳ Pending: {pending:,}")
                print(f"  🚀 Crawling: {crawling:,}")
                print(f"  ❌ Failed: {failed:,}")
                print(f"  📧 Emails Sent: {emails_sent:,}")
                print()
                print("🔄 **JOB QUEUE STATUS**")
                print(f"  ⏳ Queue Pending: {queue_pending:,}")
                print(f"  ⚡ Processing: {queue_processing:,}")
                print(f"  ✅ Queue Completed: {queue_completed:,}")
                print(f"  ❌ Queue Failed: {queue_failed:,}")
                print()
                
                # Progress bar
                if total_domains > 0:
                    progress_percent = (completed / total_domains) * 100
                    bar_length = 50
                    filled_length = int(bar_length * progress_percent // 100)
                    bar = '█' * filled_length + '-' * (bar_length - filled_length)
                    print(f"📈 Progress: |{bar}| {progress_percent:.1f}%")
                
                print("=" * 80)
                print("Press Ctrl+C to stop monitoring")
                
                # Check if completed
                if pending == 0 and crawling == 0 and queue_processing == 0:
                    print("\n🎉 **CRAWLING COMPLETED!**")
                    print(f"📊 Final Results:")
                    print(f"  ✅ Successful: {completed:,} domains")
                    print(f"  ❌ Failed: {failed:,} domains")
                    print(f"  📧 Emails sent: {emails_sent:,}")
                    print(f"  ⏰ Total time: {elapsed_time/60:.1f} minutes")
                    break
                
                last_completed = completed
                time.sleep(check_interval)
                
            except KeyboardInterrupt:
                print("\n⏹️  Monitoring stopped by user")
                break
            except Exception as e:
                print(f"❌ Error during monitoring: {e}")
                time.sleep(check_interval)
    
    def retry_failed_jobs(self):
        """Retry failed crawl and email jobs"""
        print("🔄 Retrying failed jobs...")
        
        response = requests.post(f"{self.api_base}/domains/jobs/retry")
        
        if response.status_code == 200:
            result = response.json()
            if result.get('success'):
                retried_count = result.get('data', {}).get('retriedCount', 0)
                print(f"✅ Successfully retried {retried_count} failed jobs")
            else:
                print(f"❌ Failed to retry jobs: {result.get('message')}")
        else:
            print(f"❌ HTTP Error {response.status_code} retrying jobs")
    
    def export_results(self, output_file: str = "crawl_results.csv"):
        """Export crawl results to CSV"""
        print(f"📤 Exporting results to {output_file}...")
        
        response = requests.get(f"{self.api_base}/domains/export")
        
        if response.status_code == 200:
            with open(output_file, 'w') as file:
                file.write(response.text)
            print(f"✅ Results exported to {output_file}")
        else:
            print(f"❌ HTTP Error {response.status_code} exporting results")

def main():
    parser = argparse.ArgumentParser(description='Bulk Domain Crawling Tool')
    parser.add_argument('csv_file', help='CSV file containing domains and webmaster emails')
    parser.add_argument('--base-url', default='http://localhost:3000', help='Base URL of the search engine API')
    parser.add_argument('--max-concurrent', type=int, default=10, help='Maximum concurrent crawl sessions')
    parser.add_argument('--monitor-interval', type=int, default=30, help='Monitoring check interval in seconds')
    parser.add_argument('--no-monitor', action='store_true', help='Skip monitoring after starting crawl')
    
    args = parser.parse_args()
    
    crawler = BulkCrawler(args.base_url)
    
    print("🚀 **BULK DOMAIN CRAWLER** 🚀")
    print("=" * 50)
    
    # Step 1: Import domains
    if not crawler.import_domains_from_csv(args.csv_file):
        print("❌ Failed to import domains. Exiting.")
        return 1
    
    # Step 2: Start bulk crawl
    batch_id = crawler.start_bulk_crawl(args.max_concurrent)
    if not batch_id:
        print("❌ Failed to start bulk crawl. Exiting.")
        return 1
    
    # Step 3: Monitor progress (unless disabled)
    if not args.no_monitor:
        crawler.monitor_progress(args.monitor_interval)
        
        # Step 4: Export results
        crawler.export_results(f"crawl_results_{int(time.time())}.csv")
    else:
        print("✅ Bulk crawl started successfully!")
        print("💡 Use --monitor-interval to track progress, or check the web dashboard")
    
    print("🎉 Bulk crawling process completed!")
    return 0

if __name__ == "__main__":
    exit(main())
