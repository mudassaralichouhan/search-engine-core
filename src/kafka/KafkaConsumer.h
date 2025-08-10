#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <librdkafka/rdkafka.h>

class KafkaConsumer {
public:
    using MessageHandler = std::function<bool(const std::string& topic, const void* payload, size_t len, const std::string& key)>;

    KafkaConsumer(const std::string& bootstrapServers,
                  const std::string& groupId,
                  const std::vector<std::string>& topics,
                  MessageHandler onMessage);
    ~KafkaConsumer();

    KafkaConsumer(const KafkaConsumer&) = delete;
    KafkaConsumer& operator=(const KafkaConsumer&) = delete;

    void start();
    void stop();

private:
    static void rebalance_cb(rd_kafka_t* rk, void* opaque, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t* partitions);

    void pollLoop();

    rd_kafka_conf_t* conf_ = nullptr;
    rd_kafka_t* rk_ = nullptr;

    std::vector<std::string> topics_;
    MessageHandler onMessage_;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};


