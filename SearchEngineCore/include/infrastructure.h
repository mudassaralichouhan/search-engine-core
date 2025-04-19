#pragma once
#include <string>

template <typename T>
struct Result {
    bool success;
    T value;
    std::string message;

    static Result<T> Success(T value, const std::string& message) {
        return { true, value, message };
    }

    static Result<T> Failure(const std::string& message) {
        return { false, T{}, message };
    }



    Result(bool success, const T& value, const std::string& message)
        : success(success), value(value), message(message)
    {
    }

    Result() = default;
};