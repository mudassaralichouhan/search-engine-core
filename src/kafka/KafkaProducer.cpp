#include "KafkaProducer.h"
#include <librdkafka/rdkafka.h>
#include <cstring>

namespace {
inline void set_conf(rd_kafka_conf_t* conf, const char* name, const char* value) {
    char errstr[512];
    if (rd_kafka_conf_set(conf, name, value, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        // Best-effort: log to stderr
        fprintf(stderr, "librdkafka conf set failed: %s=%s (%s)\n", name, value, errstr);
    }
}
}

KafkaProducer::KafkaProducer(const std::string& bootstrapServers,
                             const std::string& clientId,
                             DeliveryCallback onDelivery)
    : deliveryCb_(std::move(onDelivery)) {
    conf_ = rd_kafka_conf_new();
    set_conf(conf_, "bootstrap.servers", bootstrapServers.c_str());
    set_conf(conf_, "client.id", clientId.c_str());
    set_conf(conf_, "enable.idempotence", "true");
    set_conf(conf_, "acks", "all");
    set_conf(conf_, "compression.type", "zstd");
    set_conf(conf_, "linger.ms", "10");
    set_conf(conf_, "batch.num.messages", "1000");
    set_conf(conf_, "message.send.max.retries", "2147483647");
    set_conf(conf_, "max.in.flight.requests.per.connection", "5");
    rd_kafka_conf_set_dr_msg_cb(conf_, &KafkaProducer::dr_msg_cb);
    rd_kafka_conf_set_opaque(conf_, this);

    char errstr[512];
    rk_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf_, errstr, sizeof(errstr));
    if (!rk_) {
        fprintf(stderr, "Failed to create Kafka producer: %s\n", errstr);
        throw std::runtime_error("Failed to create Kafka producer");
    }
    // conf_ is now owned by rk_
    conf_ = nullptr;

    sender_ = std::thread(&KafkaProducer::senderLoop, this);
}

KafkaProducer::~KafkaProducer() {
    stop_.store(true);
    cv_.notify_all();
    if (sender_.joinable()) sender_.join();
    if (rk_) {
        rd_kafka_flush(rk_, 10000);
        rd_kafka_destroy(rk_);
        rk_ = nullptr;
    }
    if (conf_) rd_kafka_conf_destroy(conf_);
}

bool KafkaProducer::enqueue(Message msg) {
    if (stop_.load()) return false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(msg));
    }
    cv_.notify_one();
    return true;
}

bool KafkaProducer::produce(const Message& msg, int /*timeoutMs*/) {
    rd_kafka_resp_err_t err = rd_kafka_producev(
        rk_,
        RD_KAFKA_V_TOPIC(msg.topic.c_str()),
        RD_KAFKA_V_KEY(msg.key.data(), static_cast<size_t>(msg.key.size())),
        RD_KAFKA_V_VALUE(const_cast<void*>(static_cast<const void*>(msg.payload.data())), static_cast<size_t>(msg.payload.size())),
        RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
        RD_KAFKA_V_END);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        fprintf(stderr, "producev failed: %s\n", rd_kafka_err2str(err));
        return false;
    }
    rd_kafka_poll(rk_, 0);
    return true;
}

void KafkaProducer::flush(int timeoutMs) {
    if (rk_) rd_kafka_flush(rk_, timeoutMs);
}

void KafkaProducer::dr_msg_cb(rd_kafka_t* /*rk*/, const rd_kafka_message_t* m, void* opaque) {
    auto* self = static_cast<KafkaProducer*>(opaque);
    bool ok = (m->err == RD_KAFKA_RESP_ERR_NO_ERROR);
    if (self && self->deliveryCb_) {
        const char* topic = rd_kafka_topic_name(m->rkt);
        const char* keyp = static_cast<const char*>(m->key);
        int32_t partition = m->partition;
        int64_t offset = m->offset;
        std::string key = keyp ? std::string(keyp, m->key_len) : std::string();
        self->deliveryCb_(ok, topic ? topic : std::string(), partition, offset, key);
    }
}

void KafkaProducer::senderLoop() {
    while (!stop_.load()) {
        Message msg;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(50), [&]{ return stop_.load() || !queue_.empty(); });
            if (stop_.load() && queue_.empty()) break;
            if (queue_.empty()) {
                rd_kafka_poll(rk_, 0);
                continue;
            }
            msg = std::move(queue_.front());
            queue_.pop();
        }
        (void)produce(msg);
        rd_kafka_poll(rk_, 0);
    }
}


