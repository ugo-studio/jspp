#pragma once

#include "types.hpp"
#include <vector>
#include <functional>
#include <memory>
#include <variant>
#include <coroutine>
#include <unordered_map>
#include <string>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    enum class PromiseStatus { Pending, Fulfilled, Rejected };

    struct PromiseState
    {
        PromiseStatus status = PromiseStatus::Pending;
        std::shared_ptr<AnyValue> result; // Value if fulfilled, reason if rejected
        std::vector<std::function<void(const AnyValue&)>> onFulfilled;
        std::vector<std::function<void(const AnyValue&)>> onRejected;
        
        PromiseState(); // Defined in helpers
    };

    struct JsPromisePromiseType; // Forward declaration

    struct JsPromise
    {
        using promise_type = JsPromisePromiseType;

        std::shared_ptr<PromiseState> state;
        std::unordered_map<std::string, AnyValue> props;

        JsPromise();

        // --- Promise Logic ---
        void resolve(const AnyValue& value);
        void reject(const AnyValue& reason);
        void then(std::function<void(const AnyValue&)> onFulfilled, std::function<void(const AnyValue&)> onRejected = nullptr);
        
        // --- Methods ---
        std::string to_std_string() const;
        AnyValue get_property(const std::string& key, const AnyValue& thisVal);
        AnyValue set_property(const std::string& key, const AnyValue& value, const AnyValue& thisVal);
    };

    struct JsPromisePromiseType {
        JsPromise promise;

        JsPromise get_return_object() { return promise; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        
        void return_value(const AnyValue& val);
        
        void unhandled_exception();
        
        // await_transform for AnyValue
        auto await_transform(const AnyValue& value);
    };
}
