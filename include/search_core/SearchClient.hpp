#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <stdexcept>
#include <vector>

namespace hatef::search {

struct RedisConfig {
    std::string uri = "tcp://127.0.0.1:6379";
    std::size_t pool_size = 4;
};

class SearchError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class SearchClient {
public:
    explicit SearchClient(RedisConfig cfg = {});
    SearchClient(const SearchClient&)            = delete;
    SearchClient& operator=(const SearchClient&) = delete;
    SearchClient(SearchClient&&) noexcept        = default;
    SearchClient& operator=(SearchClient&&) noexcept = default;
    ~SearchClient();

    /// Execute a raw FT.SEARCH call. Returns JSON string from Redis.
    std::string search(std::string_view index,
                       std::string_view query,
                       const std::vector<std::string>& args = {});

private:
    struct Impl;
    std::unique_ptr<Impl> p_;  // PIMPL hides redis-plus-plus headers
};

} // namespace hatef::search 