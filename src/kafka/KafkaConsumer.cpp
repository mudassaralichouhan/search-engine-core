#include "KafkaConsumer.h"
#include <librdkafka/rdkafka.h>
#include <stdexcept>
#include <cstring>

namespace {
inline void set_conf(rd_kafka_conf_t* conf, const char* name, const char* value) {
    char errstr[512];
    if (rd_kafka_conf_set(conf, name, value, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "librdkafka conf set failed: %s=%s (%s)\n", name, value, errstr);
    }
}
}

KafkaConsumer::KafkaConsumer(const std::string& bootstrapServers,
                             const std::string& groupId,
                             const std::vector<std::string>& topics,
                             MessageHandler onMessage)
    : topics_(topics), onMessage_(std::move(onMessage)) {
    conf_ = rd_kafka_conf_new();
    set_conf(conf_, "bootstrap.servers", bootstrapServers.c_str());
    set_conf(conf_, "group.id", groupId.c_str());
    set_conf(conf_, "enable.auto.commit", "false");
    set_conf(conf_, "auto.offset.reset", "earliest");
    set_conf(conf_, "partition.assignment.strategy", "cooperative-sticky");

    char errstr[512];
    rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf_, errstr, sizeof(errstr));
    if (!rk_) throw std::runtime_error(std::string("Failed to create consumer: ") + errstr);
    conf_ = nullptr;

    if (rd_kafka_poll_set_consumer(rk_) != RD_KAFKA_RESP_ERR_NO_ERROR) {
        throw std::runtime_error("Failed to set consumer poll");
    }
}

KafkaConsumer::~KafkaConsumer() {
    stop();
    if (rk_) {
        rd_kafka_destroy(rk_);
        rk_ = nullptr;
    }
    if (conf_) rd_kafka_conf_destroy(conf_);
}

void KafkaConsumer::start() {
    rd_kafka_topic_partition_list_t* topicList = rd_kafka_topic_partition_list_new(static_cast<int>(topics_.size()));
    for (const auto& t : topics_) rd_kafka_topic_partition_list_add(topicList, t.c_str(), RD_KAFKA_PARTITION_UA);
    auto err = rd_kafka_subscribe(rk_, topicList);
    rd_kafka_topic_partition_list_destroy(topicList);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) throw std::runtime_error("subscribe failed");
    stop_.store(false);
    thread_ = std::thread(&KafkaConsumer::pollLoop, this);
}

void KafkaConsumer::stop() {
    if (!rk_) return;
    stop_.store(true);
    if (thread_.joinable()) thread_.join();
    rd_kafka_unsubscribe(rk_);
}

void KafkaConsumer::pollLoop() {
    while (!stop_.load()) {
        rd_kafka_message_t* m = rd_kafka_consumer_poll(rk_, 200);
        if (!m) continue;
        if (m->err) {
            if (m->err != RD_KAFKA_RESP_ERR__PARTITION_EOF && m->err != RD_KAFKA_RESP_ERR__TIMED_OUT)
                fprintf(stderr, "consumer error: %s\n", rd_kafka_message_errstr(m));
            rd_kafka_message_destroy(m);
            continue;
        }
        const char* topic = rd_kafka_topic_name(m->rkt);
        const void* payload = m->payload;
        size_t len = static_cast<size_t>(m->len);
        const char* keyp = static_cast<const char*>(m->key);
        std::string key = keyp ? std::string(keyp, m->key_len) : std::string();

        bool ok = false;
        try {
            ok = onMessage_ ? onMessage_(topic ? topic : std::string(), payload, len, key) : true;
        } catch (...) { ok = false; }

        if (ok) {
            rd_kafka_commit_message(rk_, m, /*async=*/0);
        }
        rd_kafka_message_destroy(m);
    }
}


