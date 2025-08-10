#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <librdkafka/rdkafka.h>

// Minimal RAII Kafka producer using librdkafka C API
class KafkaProducer {
public:
    struct Message {
        std::string topic;
        std::string key;
        std::string payload;
    };

    using DeliveryCallback = std::function<void(bool /*ok*/, const std::string& /*topic*/, int32_t /*partition*/, int64_t /*offset*/, const std::string& /*key*/)>;

    KafkaProducer(const std::string& bootstrapServers,
                  const std::string& clientId,
                  DeliveryCallback onDelivery = nullptr);
    ~KafkaProducer();

    // Non-copyable
    KafkaProducer(const KafkaProducer&) = delete;
    KafkaProducer& operator=(const KafkaProducer&) = delete;

    // Thread-safe enqueue; returns false if shutting down
    bool enqueue(Message msg);

    // Synchronous one-shot send (bypasses internal queue)
    bool produce(const Message& msg, int timeoutMs = 5000);

    // Force flush remaining messages before shutdown
    void flush(int timeoutMs = 10000);

private:
    static void dr_msg_cb(rd_kafka_t* rk, const rd_kafka_message_t* rkmessage, void* opaque);

    void senderLoop();

    rd_kafka_conf_t* conf_ = nullptr;
    rd_kafka_t* rk_ = nullptr;

    DeliveryCallback deliveryCb_;

    std::atomic<bool> stop_{false};
    std::thread sender_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Message> queue_;
};


