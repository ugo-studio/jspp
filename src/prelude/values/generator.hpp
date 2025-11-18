#pragma once

#include "types.hpp"
#include <coroutine>
#include <optional>
#include <iostream>
#include <utility>
#include <exception>

namespace jspp
{
    // Forward declaration of AnyValue
    class AnyValue;

    template <typename T>
    class JsGenerator
    {
    public:
        struct NextResult
        {
            std::optional<T> value;
            bool done;
        };
        struct promise_type
        {
            std::optional<T> current_value;
            std::exception_ptr exception_;

            JsGenerator get_return_object()
            {
                return JsGenerator{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            // Optimized yield_value using perfect forwarding
            template <typename From>
            std::suspend_always yield_value(From &&from)
            {
                current_value = std::forward<From>(from);
                return {};
            }

            void return_void() {}

            void unhandled_exception()
            {
                exception_ = std::current_exception();
            }
        };

        using handle_type = std::coroutine_handle<promise_type>;
        handle_type handle;

        explicit JsGenerator(handle_type h) : handle(h) {}
        JsGenerator(JsGenerator &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}
        JsGenerator(const JsGenerator &) = delete;
        JsGenerator &operator=(const JsGenerator &) = delete;

        ~JsGenerator()
        {
            if (handle)
                handle.destroy();
        }

        NextResult next()
        {
            if (!handle || handle.done())
                return {std::nullopt, true};

            handle.resume();

            if (handle.promise().exception_)
            {
                std::rethrow_exception(handle.promise().exception_);
            }

            if (handle.done())
                return {std::nullopt, true};

            // Move the value out of the promise to avoid a copy
            return {std::move(handle.promise().current_value), false};
        }

        std::unordered_map<std::string, AnyValue> props;

        std::string to_std_string() const;
        AnyValue get_property(const std::string &key);
        AnyValue set_property(const std::string &key, const AnyValue &value);
    };

}